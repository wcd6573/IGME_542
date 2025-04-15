/*
William Duprey
4/14/25
Particle Vertex Shader
*/

cbuffer ExternalData : register(b0)
{
    float currentTime;
}

// Struct for particle data needed to render it, 
// and simulate parts of it
// - Must match C++ struct
struct Particle
{
    float EmitTime;
    float3 StartPosition;
};

// Buffer of particle data
StructuredBuffer<Particle> ParticleData : register(t0);

float4 main( float4 pos : POSITION ) : SV_POSITION
{
    // Grab one particle and calculate its age
    Particle p = ParticleData.Load(particleID);
    float age = currentTime - p.EmitTime;

}