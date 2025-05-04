/*
William Duprey
5/2/25
Screen Space Ambient Occlusion Pixel Shader
*/

cbuffer ExternalData : register(b0)
{
    matrix viewMatrix;		 // Camera view matrix
    matrix projectionMatrix; // Camera projection matrix
    matrix invProjMatrix;	 // Inverse of projection matrix
	
    float4 ssaoOffsets[64];	 // Random offsets from C++
    float ssaoRadius;        // Controllable from C++
    int ssaoSamples;         // No more than above array size
    float2 randomTextureScreenScale; // (windowWidth/4.0, windowHeight/4.0)
}
	
struct VertexToPixel 
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

Texture2D Normals : register(t1);
Texture2D Depths : register(t2);
Texture2D Random : register(t3);

SamplerState BasicSampler : register(s0);
SamplerState ClampSampler : register(s1);

// --------------------------------------------------------
// Helper function copied from SSAO slides
// Takes depth and uv coords, converts to the view space
// position at that surface. Used when reading from the
// depth render target.
// --------------------------------------------------------
float3 ViewSpaceFromDepth(float depth, float2 uv)
{
    // Back to NDCs
    uv.y = 1.0f - uv.y; // Invert Y due to UV <--> NDC diff
    uv = uv * 2.0f - 1.0f;
    float4 screenPos = float4(uv, depth, 1.0f);

    // Back to view space
    float4 viewPos = mul(invProjMatrix, screenPos);
    return viewPos.xyz / viewPos.w;
}

// --------------------------------------------------------
// Helper function copied from SSAO slides
// Essentially opposite of above. Given a posiiton, get
// the UV coord on the screen. Used to sample depth buffer
// at nearby pixels.
// --------------------------------------------------------
float2 UVFromViewSpacePosition(float3 viewSpacePosition)
{
    // Apply the projection matrix to the view space 
    // position, then perspective divide
    float4 samplePosScreen = mul(projectionMatrix, float4(viewSpacePosition, 1));
    samplePosScreen.xyz /= samplePosScreen.w;
    
    // Adjust from NDCs to UV coords (flip the Y)
    samplePosScreen.xy = samplePosScreen.xy * 0.5f + 0.5f;
    samplePosScreen.y = 1.0f - samplePosScreen.y;
    
    // Return just the UVs
    return samplePosScreen.xy;
}

// --------------------------------------------------------
// Main function to calculate screen space ambient occlusion.
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
    float pixelDepth = Depths.Sample(ClampSampler, input.uv).r;
    if (pixelDepth == 1.0f)
        return float4(1, 1, 1, 1);
    
    // Get the view space position of this pixel
    float3 pixelPositionViewSpace = ViewSpaceFromDepth(pixelDepth, input.uv);
    
    // Sample the 4x4 random texture 
    // (assuming it holds already normalized vector3's)
    float3 randomDir = Random.Sample(BasicSampler, input.uv * randomTextureScreenScale).xyz;

    // Sample normal and convert to view space
    float3 normal = Normals.Sample(BasicSampler, input.uv).xyz * 2 - 1;
    normal = normalize(mul((float3x3) viewMatrix, normal));
    
    // Calculate TBN matrix
    float3 tangent = normalize(randomDir - normal * dot(randomDir, normal));
    float3 bitangent = cross(tangent, normal);
    float3x3 TBN = float3x3(tangent, bitangent, normal);
    
    // Loop and check near-by pixels for occluders
    float ao = 0.0f;
    for (int i = 0; i < ssaoSamples; i++)
    {
        // Rotate the offset, scale, and apply to position
        float3 samplePosView = pixelPositionViewSpace 
            + mul(ssaoOffsets[i].xyz, TBN) * ssaoRadius;
        
        // Get the UV coord of this position
        float2 samplePosScreen = UVFromViewSpacePosition(samplePosView);
        
        // Sample hte nearby depth and convert to view space
        float sampleDepth = Depths.SampleLevel(ClampSampler, samplePosScreen.xy, 0).r;
        float sampleZ = ViewSpaceFromDepth(sampleDepth, samplePosScreen.xy).z;
        
        // Compare the depths and fade result based on range 
        // (so far away objects aren't occluded)
        float rangeCheck = smoothstep(0.0f, 1.0f, ssaoRadius / abs(pixelPositionViewSpace.z - sampleZ));
        ao += (sampleZ < samplePosView.z ? rangeCheck : 0.0f);
    }
    
    // Average the results and flip, then return as a greyscale color
    // - Really only need to return a single number,
    //   but this lets ImGui display it properly
    ao = 1.0f - ao / ssaoSamples;
    return float4(ao.rrr, 1);
}