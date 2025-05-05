/*
William Duprey
5/4/25
Screen Space Ambient Occlusion Combine Pixel Shader
*/

cbuffer ExternalData : register(b0)
{
    int ssaoOn;
    int ssaoOnly;
}

struct VertexToPixel
{
	float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

Texture2D SceneColors     : register(t0);
Texture2D Ambient         : register(t1);
Texture2D SSAOBlur        : register(t2);
SamplerState BasicSampler : register(s0);

// --------------------------------------------------------
// Main function to perform the final SSAO render, which
// combines the scene, ambient, and blurred SSAO results
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
    // Sample all three textures
    float3 sceneColors = SceneColors.Sample(BasicSampler, input.uv).rgb;
    float3 ambient = Ambient.Sample(BasicSampler, input.uv).rgb;
    float ao = SSAOBlur.Sample(BasicSampler, input.uv).r;
    
    // If ssao is off, set ao to 1, 
    // since then multiplication is nothing
    if (!ssaoOn)
    {
        ao = 1.0f;
    }
    
    // Exit early with only ambient occlusion
    if (ssaoOnly)
    {
        return float4(ao, ao, ao, 1);
    }
        
    // Final combine
    return float4(ambient * ao + sceneColors, 1);
}