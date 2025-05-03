/*
William Duprey
5/2/25
Screen Space Ambient Occlusion Pixel Shader
*/

cbuffer ExternalData : register(b0)
{
    float4 ssaoOffsets[64];
}
	
struct VertexToPixel 
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

Texture2D Pixels : register(t0);
SamplerState ClampSampler : register(s0);

// For now, just return the texture color for the current pixel
float4 main(VertexToPixel input) : SV_TARGET
{
	return Pixels.Sample(ClampSampler, input.uv);
}