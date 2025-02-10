/*
William Duprey
2/9/25
Basic Pixel Shader
*/

#include "ShaderIncludes.hlsli"

Texture2D Albedo		: register(t0);
Texture2D MetalnessMap	: register(t1);
Texture2D NormalMap		: register(t2);
Texture2D RoughnessMap	: register(t3);
SamplerState BasicSampler : register(s0);

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
	// Sample a texture and return the color
	float3 albedoColor = Albedo.Sample(BasicSampler, input.uv).rgb;
	return float4(albedoColor, 1.0f);

}