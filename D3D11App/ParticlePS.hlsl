/*
William Duprey
4/16/25
Particle Pixel Shader
*/

cbuffer externalData : register(b0)
{
    
};

struct VertexToPixel
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 colorTint : COLOR;
};

Texture2D Particle : register(t0);
SamplerState BasicSampler : register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
    // Sample texture
    float4 color = Particle.Sample(BasicSampler, input.uv) * input.colorTint;
    
    // Return particle color
    return color;
}