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

// --------------------------------------------------------
// Creates the raytracing pipeline state, which holds
// information about the shaders, payload, root signatures, etc.
// --------------------------------------------------------
void RayTracing::CreateRaytracingPipelineState(std::wstring raytracingShaderLibraryFile)
{
    // Don't bother if DXR isn't available
    if (dxrInitialized || !dxrAvailable) { return; }

    // Read the pre-compiled shader library to a blob
    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    D3DReadFileToBlob(raytracingShaderLibraryFile.c_str(), blob.GetAddressOf());

    // There are ten subobjects that make up our raytracing pipeline object:
    // - Ray generation shader
    // - Miss shader
    // - Closest hit shader
    // - Hit group (group of all "hit"-type shaders, which is just "closest hit" for us
    // - Payload configuration
    // - Association of payload to shaders
    // - Local root signature
    // - Association of local root sig to shader
    // - Global root signature
    // - Overall pipeline config
    D3D12_STATE_SUBOBJECT subobjects[10] = {};

    // === Ray generation shader ===
    D3D12_EXPORT_DESC rayGenExportDesc = {};
    rayGenExportDesc.Name = L"RayGen";
    rayGenExportDesc.Flags = D3D12_EXPORT_FLAG_NONE;

    D3D12_DXIL_LIBRARY_DESC rayGenLibDesc = {};
    rayGenLibDesc.DXILLibrary.BytecodeLength = blob->GetBufferSize();
    rayGenLibDesc.DXILLibrary.pShaderBytecode = blob->GetBufferPointer();
    rayGenLibDesc.NumExports = 1;
    rayGenLibDesc.pExports = &rayGenExportDesc;

    D3D12_STATE_SUBOBJECT rayGenSubObj = {};
    rayGenSubObj.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    rayGenSubObj.pDesc = &rayGenLibDesc;

    subobjects[0] = rayGenSubObj;

    // === Miss shader ===
    D3D12_EXPORT_DESC missExportDesc = {};
    missExportDesc.Name = L"Miss";
    missExportDesc.Flags = D3D12_EXPORT_FLAG_NONE;

    D3D12_DXIL_LIBRARY_DESC missLibDesc = {};
    missLibDesc.DXILLibrary.BytecodeLength = blob->GetBufferSize();
    missLibDesc.DXILLibrary.pShaderBytecode = blob->GetBufferPointer();
    missLibDesc.NumExports = 1;
    missLibDesc.pExports = &missExportDesc;

    D3D12_STATE_SUBOBJECT missSubObj = {};
    missSubObj.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    missSubObj.pDesc = &missLibDesc;

    subobjects[1] = missSubObj;

    // === Closest hit shader ===
    D3D12_EXPORT_DESC closestHitExportDesc = {};
    closestHitExportDesc.Name = L"ClosestHit";
    closestHitExportDesc.Flags = D3D12_EXPORT_FLAG_NONE;

    D3D12_DXIL_LIBRARY_DESC closestHitLibDesc = {};
    closestHitLibDesc.DXILLibrary.BytecodeLength = blob->GetBufferSize();
    closestHitLibDesc.DXILLibrary.pShaderBytecode = blob->GetBufferPointer();
    closestHitLibDesc.NumExports = 1;
    closestHitLibDesc.pExports = &closestHitExportDesc;

    D3D12_STATE_SUBOBJECT closestHitSubObj = {};
    closestHitSubObj.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    closestHitSubObj.pDesc = &closestHitLibDesc;

    subobjects[2] = closestHitSubObj;

    // === Hit group ===
    D3D12_HIT_GROUP_DESC hitGroupDesc = {};
    hitGroupDesc.ClosestHitShaderImport = L"ClosestHit";
    hitGroupDesc.HitGroupExport = L"HitGroup";

    D3D12_STATE_SUBOBJECT hitGroup = {};
    hitGroup.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    hitGroup.pDesc = &hitGroupDesc;

    subobjects[3] = hitGroup;

    // === Shader config ===
    D3D12_RAYTRACING_SHADER_CONFIG shaderConfigDesc = {};
    // Assuming float3 color, and float2 for barycentric coordinates
    shaderConfigDesc.MaxPayloadSizeInBytes = sizeof(DirectX::XMFLOAT3);
    shaderConfigDesc.MaxAttributeSizeInBytes = sizeof(DirectX::XMFLOAT2);

    D3D12_STATE_SUBOBJECT shaderConfigSubObj = {};
    shaderConfigSubObj.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
    shaderConfigSubObj.pDesc = &shaderConfigDesc;

    subobjects[4] = shaderConfigSubObj;

    // === Association - Payload and shaders ===
    // Names of shaders that use the payload
    const wchar_t* payloadShaderNames[] = { L"RayGen", L"Miss", L"HitGroup" };
    
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderPayloadAssociation = {};
    shaderPayloadAssociation.NumExports = ARRAYSIZE(payloadShaderNames);
    shaderPayloadAssociation.pExports = payloadShaderNames;
    shaderPayloadAssociation.pSubobjectToAssociate = &subobjects[4]; // Payload config above

    D3D12_STATE_SUBOBJECT shaderPayloadAssociationObject = {};
    shaderPayloadAssociationObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
    shaderPayloadAssociationObject.pDesc = &shaderPayloadAssociation;

    subobjects[5] = shaderPayloadAssociationObject;

    // === Local root signature ===
    D3D12_STATE_SUBOBJECT localRootSigSubObj = {};
    localRootSigSubObj.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
    localRootSigSubObj.pDesc = LocalRaytracingRootSig.GetAddressOf();

    subobjects[6] = localRootSigSubObj;

    // === Association - Shaders and local root sig ===
    // Names of shaders that use the root sig
    const wchar_t* rootSigShaderNames[] = { L"RayGen", L"Miss", L"HitGroup" };

    // Add a state subobject for the association between
    // the RayGen shader and the local root signature
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION rootSigAssociation = {};
    rootSigAssociation.NumExports = ARRAYSIZE(rootSigShaderNames);
    rootSigAssociation.pExports = rootSigShaderNames;
    rootSigAssociation.pSubobjectToAssociate = &subobjects[6]; // Root sig above

    D3D12_STATE_SUBOBJECT rootSigAssociationSubObj = {};
    rootSigAssociationSubObj.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
    rootSigAssociationSubObj.pDesc = &rootSigAssociation;

    subobjects[7] = rootSigAssociationSubObj;

    // === Global root sig ===
    D3D12_STATE_SUBOBJECT globalRootSigSubObj = {};
    globalRootSigSubObj.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
    rootSigAssociationSubObj.pDesc = GlobalRaytracingRootSig.GetAddressOf();

    subobjects[8] = globalRootSigSubObj;

    // === Pipeline config ===
    // Add a state subobject for the ray tracing pipeline config
    D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
    pipelineConfig.MaxTraceRecursionDepth = D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH;

    D3D12_STATE_SUBOBJECT pipelineConfigSubObj = {};
    pipelineConfigSubObj.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
    rootSigAssociationSubObj.pDesc = &pipelineConfig;

    subobjects[9] = pipelineConfigSubObj;

    // === Finalize state ===
    D3D12_STATE_OBJECT_DESC raytracingPipelineDesc = {};
    raytracingPipelineDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    raytracingPipelineDesc.NumSubobjects = ARRAYSIZE(subobjects);
    raytracingPipelineDesc.pSubobjects = subobjects;

    // Create the state and also query it for its properties
    DXRDevice->CreateStateObject(
        &raytracingPipelineDesc, 
        IID_PPV_ARGS(RaytracingPipelineStateObject.GetAddressOf()));
    RaytracingPipelineStateObject->QueryInterface(
        IID_PPV_ARGS(&RaytracingPipelineProperties));
}


// --------------------------------------------------------
// Sets up the shader table, which holds shader identifiers
// and local root signatures for all possible shaders
// used during raytracing. Note that this is just a big
// chunk of GPU memory we need to manage ourselves.
// --------------------------------------------------------
void RayTracing::CreateShaderTable()
{
    // Don't bother if DXR isn't available
    if (dxrInitialized || !dxrAvailable) { return; }

    // Create the table of shaders and their data to use for rays
    // 0 - Ray generation shader
    // 1 - Miss shader
    // 2 - Closest hit shader
    // Note: All records must have the same size, so we need to calculate
    //       the size of the largest possible entry for our program
    //       - This will be the default (32) + one descriptor table pointer (8)
    //       - This also must be aligned up to 
    //         D3D12_RAYTRACING_SHADER_BINDING_TABLE_RECORD_BYTE_ALIGNMENT.
    UINT64 shaderTableRayGenRecordSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    UINT64 shaderTableMissRecordSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    UINT64 shaderTableHitGroupRecordSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES 
        + sizeof(D3D12_GPU_DESCRIPTOR_HANDLE) * 2; // CBV & SRV

    // Align them
    shaderTableRayGenRecordSize = ALIGN(shaderTableRayGenRecordSize, 
        D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
    shaderTableMissRecordSize = ALIGN(shaderTableMissRecordSize, 
        D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
    shaderTableHitGroupRecordSize = ALIGN(shaderTableHitGroupRecordSize, 
        D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

    // Which is largest?
    ShaderTableRecordSize = max(shaderTableRayGenRecordSize, 
        max(shaderTableMissRecordSize, shaderTableHitGroupRecordSize));

    // How big should the table be? 
    // Need a record for each of our 3 shaders (in the simple demo)
    UINT64 shaderTableSize = ShaderTableRecordSize * 3;
    shaderTableSize = ALIGN(shaderTableSize, 
        D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

    // Create the shader table buffer and map it so we can write to it
    ShaderTable = Graphics::CreateBuffer(
        shaderTableSize, 
        D3D12_HEAP_TYPE_UPLOAD, 
        D3D12_RESOURCE_STATE_GENERIC_READ);
    unsigned char* shaderTableData = 0;
    ShaderTable->Map(0, 0, (void**)&shaderTableData);

    // Mem copy each record in: ray gen, miss, and the overall hit group 
    // (from CreateRaytracingPipelineState() above)
    memcpy(shaderTableData, 
        RaytracingPipelineProperties->GetShaderIdentifier(L"RayGen"), 
        D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    shaderTableData += ShaderTableRecordSize;

    memcpy(shaderTableData, 
        RaytracingPipelineProperties->GetShaderIdentifier(L"Miss"), 
        D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    shaderTableData += ShaderTableRecordSize;

    memcpy(shaderTableData,
        RaytracingPipelineProperties->GetShaderIdentifier(L"HitGroup"),
        D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    
    // We'll eventually need to memcpy per-object data to the shader table,
    // but we don't do that yet

    // Unmap
    ShaderTable->Unmap(0, 0);
}

// --------------------------------------------------------
// Creates a texture & wraps it with an Unordered Access View,
// allowing shaders to directly write into this memory. The
// data in this texture will later be directly copied to the
// back buffer after raytracing is complete.
// --------------------------------------------------------
void RayTracing::CreateRaytracingOutputUAV(unsigned int width, unsigned int height)
{
    // Default heap for output buffer
    D3D12_HEAP_PROPERTIES heapDesc = {};
    heapDesc.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapDesc.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapDesc.CreationNodeMask = 0;
    heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapDesc.VisibleNodeMask = 0;

    // Describe the final output resource (UAV)
    D3D12_RESOURCE_DESC desc = {};
    desc.DepthOrArraySize = 1;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    desc.Width = width;
    desc.Height = height;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    DXRDevice->CreateCommittedResource(
        &heapDesc,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_SOURCE,
        0,
        IID_PPV_ARGS(RaytracingOutput.GetAddressOf()));

    // Do we have a UAV already?
    if (!RaytracingOutputUAV_GPU.ptr)
    {
        // Nope, so reserve a spot
        Graphics::ReserveDescriptorHeapSlot(
            &RaytracingOutputUAV_CPU,
            &RaytracingOutputUAV_GPU);
    }

    // Set up hte UAV
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

    DXRDevice->CreateUnorderedAccessView(
        RaytracingOutput.Get(),
        0, 
        &uavDesc, 
        RaytracingOutputUAV_CPU);
}


// --------------------------------------------------------
// If the window size changes, so too should the output texture.
// --------------------------------------------------------
void RayTracing::ResizeOutputUAV(
    unsigned int outputWidth, 
    unsigned int outputHeight)
{
    if (!dxrInitialized || !dxrAvailable) { return; }

    // Wait for the GPU to be done
    Graphics::WaitForGPU();

    // Reset and re-create the buffer
    RaytracingOutput.Reset();
    CreateRaytracingOutputUAV(outputWidth, outputHeight);
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
