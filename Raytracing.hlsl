/*
William Duprey
3/29/25
Raytracing Shader Library
 - Starter code by prof. Chris Cascioli
 - Loosely followed this NVIDIA tutorial for shadows:
   https://developer.nvidia.com/rtx/raytracing/dxr/dx12-raytracing-tutorial/extra/dxr_tutorial_extra_another_ray_type
*/

#include "ShaderIncludes.hlsli"

// --- Structs ---
// Layout of data in the vertex buffer
struct Vertex
{
    float3 localPosition;
    float2 uv;
    float3 normal;
    float3 tangent;
};

// 11 floats total per vertex * 4 bytes each
static const uint VertexSizeInBytes = 11 * 4;

// Constants to control the intensity of raytracing
static const int RaysPerPixel = 5;
static const int MaxRecursionDepth = 5;


// Payload for rays (data that is "sent along" with each ray during raytrace)
// Note: This should be as small as possible, and must match our C++ size definition
struct RayPayload
{
    float3 color;
    uint recursionDepth;
    uint rayPerPixelIndex;
    bool isHit; // Used for shadows
};

// Note: We'll be using the built-in BuiltInTriangleIntersectionAttributes
// for triangle EvaluateAttributeSnapped, so no need to define our own.
// It contains a single float2.


// --- Constant Buffers ---
cbuffer SceneData : register(b0)
{
    matrix inverseViewProjection;
    float3 cameraPosition;
};

// Ensure this matches C++ buffer struct definition
#define MAX_INSTANCES_PER_BLAS 100
cbuffer ObjectData : register(b1)
{
    float4 entityColor[MAX_INSTANCES_PER_BLAS];
};

// --- Resources ---
// Output UAV
RWTexture2D<float4> OutputColor : register(u0);

// The actual scene we want to trace through (a TLAS)
RaytracingAccelerationStructure SceneTLAS : register(t0);

// Geometry buffers
ByteAddressBuffer IndexBuffer : register(t1);
ByteAddressBuffer VertexBuffer : register(t2);

// --- Helpers ---
// Loads the indices of the specified triangle from the index buffer
uint3 LoadIndices(uint triangleIndex)
{
    // What is the start index of this triangle's indices?
    uint indicesStart = triangleIndex * 3;
    
    // Adjust by the byte size before loading
    return IndexBuffer.Load3(indicesStart * 4); // 4 bytes per index
}

// Barycentric interpolation of data from the triangleIndex's vertices
Vertex InterpolateVertices(uint triangleIndex, float2 barycentrics)
{
    // Calculate the barycentric data for vertex interpolation
    float3 barycentricData = float3(
        1.0f - barycentrics.x - barycentrics.y,
        barycentrics.x,
        barycentrics.y);
    
    // Grab the indices
    uint3 indices = LoadIndices(triangleIndex);
    
    // Set up the final vertex
    Vertex vert = (Vertex)0;
    
    // Loop through the barycentric data and interpolate
    for (uint i = 0; i < 3; i++)
    {
        // Get the index of the first piece of data for this vertex
        uint dataIndex = indices[i] * VertexSizeInBytes;
        
        // Grab the position and offset
        vert.localPosition += asfloat(VertexBuffer.Load3(dataIndex)) * barycentricData[i];
        dataIndex += 3 * 4; // 3 floats * 4 bytes per float
        
        // UV
        vert.uv += asfloat(VertexBuffer.Load2(dataIndex)) * barycentricData[i];
        dataIndex += 2 * 4; // 2 floats * 4 bytes per float
        
        // Normal
        vert.normal += asfloat(VertexBuffer.Load3(dataIndex)) * barycentricData[i];
        dataIndex += 3 * 4; // 3 floats * 4 bytes per float
        
        // Tangent (no offset at the end, since we start over after looping)
        vert.tangent += asfloat(VertexBuffer.Load3(dataIndex)) * barycentricData[i];
    }

    // Final interpolated vertex data is ready
    return vert;
}

// Calculates an origin and direction from the camera for specific pixel indices
RayDesc CalcRayFromCamera(float2 rayIndices)
{
    // Offset to the middle of the pixel
    float2 pixel = rayIndices + 0.5f;
    float2 screenPos = pixel / DispatchRaysDimensions().xy * 2.0f - 1.0f;
    screenPos.y = -screenPos.y;
    
    // Unproject the coords
    float4 worldPos = mul(inverseViewProjection, float4(screenPos, 0, 1));
    worldPos.xyz /= worldPos.w;
    
    // Set up hte ray
    RayDesc ray;
    ray.Origin = cameraPosition.xyz;
    ray.Direction = normalize(worldPos.xyz - ray.Origin);
    ray.TMin = 0.01f;
    ray.TMax = 1000.0f;
    return ray;
}


// --- Shaders ---
// Ray Regeneration shader - Launched once for each ray we want to generate 
// (which is generally once per pixel of our output texture)
[shader("raygeneration")]
void RayGen()
{
    // Get the ray indices
    uint2 rayIndices = DispatchRaysIndex().xy;
 
    // Multiple rays per pixel code
    // - From the GGP2 path tracing slides
    float3 totalColor = float3(0, 0, 0);
    
    int raysPerPixel = RaysPerPixel;
    for (int r = 0; r < raysPerPixel; r++)
    {
        float2 adjustedIndices = (float2) rayIndices;
        float ray01 = (float) r / raysPerPixel;
        adjustedIndices += rand2(rayIndices.xy * ray01);
        
        // --- Raytracing work ---
        // Calculate the ray from the camera through a particular
        // pixel of the output buffer using this shader's indices
        RayDesc ray = CalcRayFromCamera(rayIndices);
    
        // Set up the payload for the ray
        // This initializes the struct to all zeroes
        RayPayload payload = (RayPayload) 0;
        payload.color = float3(1, 1, 1);
        payload.recursionDepth = 0;
        payload.rayPerPixelIndex = r;
    
        // Perform the ray trace for this ray
        TraceRay(
            SceneTLAS,
            RAY_FLAG_NONE,
            0xFF,
            0,
            0,
            0,
            ray,
            payload);        
        
        totalColor += payload.color;
    }
    
    float3 avg = totalColor / raysPerPixel;
    
    // Set the final color of the buffer
    OutputColor[rayIndices] = float4(pow(avg, 1.0f / 2.2f), 1);
}

// Miss shader - What happens if the ray doesn't hit anything?
[shader("miss")]
void Miss(inout RayPayload payload)
{
    // Nothing was hit, so return a constant color
    // - Ideally this is where we would do skybox stuff
    payload.color *= float3(0.4f, 0.6f, 0.75f);
}

// Miss shader for shadows
[shader("miss")]
void ShadowMiss(inout RayPayload payload)
{
    payload.isHit = false;
}

// Closest hit shader - Runs the first time a ray hits anything
[shader("closesthit")]
void ClosestHit(inout RayPayload payload, BuiltInTriangleIntersectionAttributes hitAttributes)
{
    // If reached max recursion, haven't hit a light source
    if (payload.recursionDepth >= MaxRecursionDepth)
    {
        payload.color = float3(0, 0, 0);
        payload.isHit = true;
        return;
    }
    
    // Otherwise, we've hit, do raytracing work
    // - Access the ID pre-defined in the BLAS,
    //   and use it to access the color in the constant buffer
    payload.color *= entityColor[InstanceID()].rgb;
    
    // Something was hit, so mark down the isHit boolean,
    // which is used in RayGen to scale down the final color
    payload.isHit = true;
    
    // Figure out which object was hit to calculate the normal
    Vertex vert = InterpolateVertices(PrimitiveIndex(), hitAttributes.barycentrics);
    
    // Don't care about translation, so convert to 3x3
    float3 normal = normalize(mul(vert.normal, (float3x3) ObjectToWorld4x3()));
    
    // Calculate a unique RNG value for this ray, 
    // based on the "uv" of this pixel and other per-ray data
    float2 uv = (float2) DispatchRaysIndex() / (float2)DispatchRaysDimensions();
    float2 rng = rand2(uv * (payload.recursionDepth + 1) 
        + payload.rayPerPixelIndex + RayTCurrent());
    
    // Generate a perfect reflection
    float3 refl = reflect(WorldRayDirection(), normal);
    
    // Generate a random bounce in the hemisphere
    float3 randomBounce = random_cosine_weighted_hemisphere(
        random_float(rng), 
        random_float(rng.yx), 
        normal);

    // Linearly interpolate based on the alpha channel    
    float3 direction = normalize(lerp(refl, randomBounce, entityColor[InstanceID()].a));
    
    // Generate new ray
    RayDesc ray;
    ray.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    ray.Direction = direction;
    ray.TMin = 0.0001f;
    ray.TMax = 1000.0f;
    
    // Recursive ray trace
    payload.recursionDepth++;
    TraceRay(
        SceneTLAS,
        RAY_FLAG_NONE,
        0xFF, 0, 0, 0,
        ray,
        payload);
    
    // --- Extra ray for shadows ---
    // Shadow ray description
    // - Identical except for direction, which is
    //   pointed towards the light source
    RayDesc shadowRay;
    shadowRay.Origin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
    
    // Hardcode light position
    // - This could be passed through a constant buffer,
    //   but I'm tired, and this is simpler
    shadowRay.Direction = normalize(float3(30, 30, 30) - shadowRay.Origin);
    shadowRay.TMin = 0.0001f;
    shadowRay.TMax = 1000.0f;
        
    // Set up shadow ray's payload
    RayPayload shadowPayload = (RayPayload) 0;
    shadowPayload.color = float3(1, 1, 1);
    shadowPayload.rayPerPixelIndex = payload.rayPerPixelIndex;
    shadowPayload.isHit = false;
    
    // Start at a really big recursion depth to trigger
    // the if statement at the start, which sets isHit to true
    shadowPayload.recursionDepth = MaxRecursionDepth;
    
    // Trace the ray towards the light source
    TraceRay(
        SceneTLAS,
        RAY_FLAG_NONE,
        0xFF,
        0,
        0,
        0,  // Offset to the shadow miss shader
            // I don't understand why this works when set to 0,
            // and not 1. How does it know to run the ShadowMiss shader?
            // Am I crazy? It works, so I guess I should just be thankful
        shadowRay,
        shadowPayload);
    
    // Scale the payload's final color based on the shadow ray results
    float factor = shadowPayload.isHit ? 0.3f : 1.0f;
    payload.color *= factor;
}