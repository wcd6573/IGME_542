/*
William Duprey
5/4/25
Screen Space Ambient Occlusion 4x4 Blur Pixel Shader
*/

struct VertexToPixel
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

cbuffer ExternalData : register(b0)
{
    float2 pixelSize;   // (1 / windowWidth, 1 / windowHeight)
}

Texture2D SSAO : register(t0);
SamplerState ClampSampler : register(s0);

// --------------------------------------------------------
// Main function to apply a 4x4 blur on SSAO results,
// smoothing out the repeated pattern (which was due to
// using a 4x4 texture for random values in the SSAO computation.
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
    float ao = 0;
    for (float x = -1.5f; x <= 1.5f; x++) // -1.5, -0.5, 0.5, 1.5
    {
        for (float y = -1.5f; y <= 1.5f; y++)
        {
            ao += SSAO.Sample(ClampSampler, float2(x, y) * pixelSize + input.uv).r;
        }
    }
    
    // Average results and return
    ao /= 16.0f; // 4x4 blur is 16 samples
    return float4(ao.rrr, 1);
}