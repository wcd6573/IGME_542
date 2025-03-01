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

void RayTracing::CreateRaytracingRootSignatures()
{
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
