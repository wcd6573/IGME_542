/*
William Duprey
4/16/25
Particle Vertex Shader
*/

cbuffer ExternalData : register(b0)
{
    // Camera matrices
    matrix view;
    matrix projection;
    
    // Particle properties
    float4 startColor;
    float4 endColor;
    
    float startSize;
    float endSize;
    float currentTime;
    float lifetime;
}

// Struct for particle data needed to render it, 
// and simulate parts of it
// - Must match C++ struct
struct Particle
{
    float EmitTime;
    float3 StartPosition;
    float3 StartVelocity;
    float pad;
};

// Buffer of particle data
StructuredBuffer<Particle> ParticleData : register(t0);

struct VertexToPixel
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 colorTint : COLOR;
};

// Entry point for our particle-specific vertex shader
// Only input is a single unique index for each vertex
VertexToPixel main( uint id : SV_VertexID )
{
    VertexToPixel output;
    
    uint particleID = id / 4; // Every group of 4 verts are one particle
    uint cornerID = id % 4;   // 0,1,2,3 = which corner of the "quad"
    
    // Grab one particle and calculate its age
    Particle p = ParticleData.Load(particleID);
    float age = currentTime - p.EmitTime;
    float ageFrac = age / lifetime;
    
    // Calculate position using age and starting velocity
    float3 pos = p.StartPosition + (p.StartVelocity * age);

    // --- Billboarding ---
    // Fill in offsets for each corner of the quad
    float2 offsets[4];
    offsets[0] = float2(-1.0f, +1.0f); // Top left
    offsets[1] = float2(+1.0f, +1.0f); // Top right
    offsets[2] = float2(+1.0f, -1.0f); // Bottom right
    offsets[3] = float2(-1.0f, -1.0f); // Bottom left
    
    // Offset position based on camera's right and up vectors
    float size = lerp(startSize, endSize, ageFrac);
    pos += float3(view._11, view._12, view._13) * (offsets[cornerID].x
        * size); // right
    
    pos += float3(view._21, view._22, view._23) * (offsets[cornerID].y
        * size); // up
    
    // Calculate output position using view and projection matrices
    matrix viewProj = mul(projection, view);
    output.position = mul(viewProj, float4(pos, 1.0f));
    
    // --- UVs ---
    float2 uvs[4];
    uvs[0] = float2(0, 0); // Top left
    uvs[1] = float2(1, 0); // Top right
    uvs[2] = float2(1, 1); // Bottom right
    uvs[3] = float2(0, 1); // Bottom left
    output.uv = uvs[cornerID];
    
    // Interpolate color based on age as a fraction of total lifetime
    output.colorTint = lerp(startColor, endColor, ageFrac);
    
    return output;
}