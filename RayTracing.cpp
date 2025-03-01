/*
William Duprey
3/1/25
Raytracing Implementation
 - Provided by prof. Chris Cascioli
*/

#include "RayTracing.h"
#include "Graphics.h"
#include "BufferStructs.h"
#include "Window.h"

#include <d3dcompiler.h>
#include <DirectXMath.h>

namespace RayTracing 
{
    // Anonymous namespace to hold variables
    // only accessible in this file
    namespace
    {
        bool dxrAvailable = false;
        bool dxrInitialized = false;

        // Error messages
        const char* errorRaytracingNotSupported = 
            "\nERROR: Raytracing not supported by current graphics device.\n(On laptops, this may be due to battery saver mode.)\n";
        const char* errorDXRDeviceQueryFailed = 
            "\nERROR: DXR Device query failed - DirectX Raytracing unavailable.\n";
        const char* errorDXRCommandListQueryFailed = 
            "\nERROR: DXR Command List query failed - DirectX Raytracing unavailable.\n";
    }
}

// Makes use of integer division to ensure we are aligned 
// to the proper multiple of "alignment"
#define ALIGN(value, alignment) (((value + alignment - 1) / alignment) * alignment)


// --------------------------------------------------------
// Check for raytracing support and create all necessary
// raytracing resources, pipeline states, etc.
// --------------------------------------------------------
HRESULT RayTracing::Initialize(
    unsigned int outputWidth, 
    unsigned int outputHeight, 
    std::wstring raytracingShaderLibraryFile)
{
    // Use CheckFeatureSupport to determine if raytracing is supported
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 rtSupport = {};
    HRESULT supportResult = Graphics::Device->CheckFeatureSupport(
        D3D12_FEATURE_D3D12_OPTIONS5,
        &rtSupport,
        sizeof(rtSupport));

    // Query to ensure we can get proper versions 
    // of the device and command list
    HRESULT dxrDeviceResult = Graphics::Device->QueryInterface(
        IID_PPV_ARGS(DXRDevice.GetAddressOf()));
    HRESULT dxrCommandListResult = Graphics::CommandList->QueryInterface(
        IID_PPV_ARGS(DXRCommandList.GetAddressOf()));

    // Check the results
    if (FAILED(supportResult) || rtSupport.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
    {
        printf("%s", errorRaytracingNotSupported);
        return supportResult;
    }
    if (FAILED(dxrDeviceResult))
    {
        printf("%s", errorDXRDeviceQueryFailed);
        return dxrDeviceResult;
    }
    if (FAILED(dxrCommandListResult))
    {
        printf("%s", errorDXRCommandListQueryFailed);
        return dxrCommandListResult;
    }

    // We have DXR support
    dxrAvailable = true;
    printf("\nDXR initialization success!\n");

    // Proceed with setup
    CreateRaytracingRootSignatures();
    CreateRaytracingPipelineState(raytracingShaderLibraryFile);
    CreateShaderTable();
    CreateRaytracingOutputUAV(outputWidth, outputHeight);

    // All set
    dxrInitialized = true;
    return S_OK;
}


// --------------------------------------------------------
// Creates the root signatures necessary for raytracing:
//  - A global signature used across all shaders
//  - A local signature used for each ray hit
// --------------------------------------------------------
void RayTracing::CreateRaytracingRootSignatures()
{
    // Don't bother if DXR isn't available
    if (dxrInitialized || !dxrAvailable) { return; }

    // Create a global root signature shared across all raytracing shaders
    {
        // Two descriptor ranges
        // 1: The output texture, which is an unordered access view
        // 2: Two separate SRVs, which are the index and vertex data of the geometry
        D3D12_DESCRIPTOR_RANGE outputUAVRange = {};
        outputUAVRange.BaseShaderRegister = 0;
        outputUAVRange.NumDescriptors = 1;
        outputUAVRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        outputUAVRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        outputUAVRange.RegisterSpace = 0;

        D3D12_DESCRIPTOR_RANGE cbufferRange = {};
        cbufferRange.BaseShaderRegister = 0;
        cbufferRange.NumDescriptors = 1;
        cbufferRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        cbufferRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        cbufferRange.RegisterSpace = 0;

        // Set up the root parameters for the global signature (of which there are four)
        // These need to maatch the shader(s) we'll be using
        D3D12_ROOT_PARAMETER rootParams[3] = {};
        {
            // First param is the UAV range for the output texture
            rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
            rootParams[0].DescriptorTable.pDescriptorRanges = &outputUAVRange;

            // Second param is an SRV for the acceleration structure
            rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
            rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            rootParams[1].Descriptor.ShaderRegister = 0;
            rootParams[1].Descriptor.RegisterSpace = 0;

            // Third is constant buffer for the overall scene
            // (camera matrices, lights, etc.)
            rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
            rootParams[2].DescriptorTable.pDescriptorRanges = &cbufferRange;
        }

        // Create the global root signature
        Microsoft::WRL::ComPtr<ID3DBlob> blob;
        Microsoft::WRL::ComPtr<ID3DBlob> errors;
        D3D12_ROOT_SIGNATURE_DESC globalRootSigDesc = {};
        globalRootSigDesc.NumParameters = ARRAYSIZE(rootParams);
        globalRootSigDesc.pParameters = rootParams;
        globalRootSigDesc.NumStaticSamplers = 0;
        globalRootSigDesc.pStaticSamplers = 0;
        globalRootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

        D3D12SerializeRootSignature(
            &globalRootSigDesc, 
            D3D_ROOT_SIGNATURE_VERSION_1, 
            blob.GetAddressOf(), 
            errors.GetAddressOf());
        DXRDevice->CreateRootSignature(
            1, 
            blob->GetBufferPointer(), 
            blob->GetBufferSize(), 
            IID_PPV_ARGS(GlobalRaytracingRootSig.GetAddressOf()));
    }

    // Create a local root signature enabling shaders to 
    // have unique data from shader tables
    {
        // Table of 2 starting at register(t1)
        D3D12_DESCRIPTOR_RANGE geometrySRVRange = {};
        geometrySRVRange.BaseShaderRegister = 1;
        geometrySRVRange.NumDescriptors = 2;
        geometrySRVRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        geometrySRVRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        geometrySRVRange.RegisterSpace = 0;

        // One parameter: Descriptor table housing the index and vertex buffer descriptors
        D3D12_ROOT_PARAMETER rootParams[1] = {};

        // Range of SRVs for geometry (verts & indices)
        rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
        rootParams[0].DescriptorTable.pDescriptorRanges = &geometrySRVRange;

        // Create the local root sig (ensure we denote it as a local sig)
        Microsoft::WRL::ComPtr<ID3DBlob> blob;
        Microsoft::WRL::ComPtr<ID3DBlob> errors;
        D3D12_ROOT_SIGNATURE_DESC localRootSigDesc = {};
        localRootSigDesc.NumParameters = ARRAYSIZE(rootParams);
        localRootSigDesc.pParameters = rootParams;
        localRootSigDesc.NumStaticSamplers = 0;
        localRootSigDesc.pStaticSamplers = 0;
        localRootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

        D3D12SerializeRootSignature(&localRootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, blob.GetAddressOf(), errors.GetAddressOf());
        DXRDevice->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(LocalRaytracingRootSig.GetAddressOf()));
    }
}

void RayTracing::ResizeOutputUAV(unsigned int outputWidth, unsigned int outputHeight)
{
}

void RayTracing::Raytrace(std::shared_ptr<Camera> camera, Microsoft::WRL::ComPtr<ID3D12Resource> currentBackBuffer)
{
}

void RayTracing::CreateBottomLevelAccelerationStructureForMesh(Mesh* mesh)
{
}

void RayTracing::CreateTopLevelAccelerationStructureForScene()
{
}

void RayTracing::CreateRaytracingPipelineState(std::wstring raytracingShaderLibraryFile)
{
}

void RayTracing::CreateShaderTable()
{
}

void RayTracing::CreateRaytracingOutputUAV(unsigned int width, unsigned int height)
{
}
