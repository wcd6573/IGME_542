/*
William Duprey
10/28/24
Shader Include File
*/

// Each .hlsli file needs a unique identifier
#ifndef __GGP_SHADER_INCLUDES__ 
#define __GGP_SHADER_INCLUDES__

#define M_PI 3.14159265359f

////////////////////////////////////////////////////////////////////////////////
// --------------------------------- STRUCTS -------------------------------- //
////////////////////////////////////////////////////////////////////////////////
// Struct representing a single vertex worth of data
// - This should match the vertex definition in our C++ code
// - By "match", I mean the size, order and number of members
// - The name of the struct itself is unimportant, but should be descriptive
// - Each variable must have a semantic, which defines its usage
struct VertexShaderInput
{ 
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
	float3 localPosition	: POSITION;     // XYZ position
	float2 uv				: TEXCOORD;		// UV
	float3 normal			: NORMAL;		// Normal vector
	float3 tangent			: TANGENT;		// Tangent vector
};

// Struct representing the data we're sending down the pipeline
// - Should match our pixel shader's input (hence the name: Vertex to Pixel)
// - At a minimum, we need a piece of data defined tagged as SV_POSITION
// - The name of the struct itself is unimportant, but should be descriptive
// - Each variable must have a semantic, which defines its usage
struct VertexToPixel
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
	float4 screenPosition	: SV_POSITION;	// XYZW position (System Value Position)
	float2 uv				: TEXCOORD;		// UV
	float3 normal			: NORMAL;		// Normal vector
	float3 tangent			: TANGENT;		// Tangent vector
	float3 worldPosition	: POSITION;		// World position
};

// Struct representing data from 
// sky vertex shader to sky pixel shader.
struct VertexToPixel_Sky
{
	float4 position : SV_POSITION;
	float3 sampleDir : DIRECTION;
};

////////////////////////////////////////////////////////////////////////////////
// --------------------------- HELPER FUNCTIONS ----------------------------- //
////////////////////////////////////////////////////////////////////////////////
// --------------------------------------------------------
// Performs a bunch of arbitrary steps 
// to produce a deterministically random float.
// --------------------------------------------------------
float random_float(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

float2 rand2(float2 uv)
{
    return float2(
		random_float(uv),
		random_float(uv.yx));
}

// --------------------------------------------------------
// Performs a bunch of arbitrary steps 
// to produce a deterministically random float2, additionally,
// uses an offset to further randomize.
// --------------------------------------------------------
inline float2 random_float2(float2 uv, float offset)
{
	// 2x2 matrix of arbitrary values
	float2x2 m = float2x2(
		15.27f, 47.63f,
		89.98f, 95.07f
	);
	
	// Do some arbitrary, random steps
	uv = frac(sin(mul(uv, m)) * 46839.32f);

	// Make a new float2, scaled by angle offset
	return float2(
		sin(uv.x * offset) * 0.5f + 0.5f,
		cos(uv.y * offset) * 0.5f + 0.5f
	);
}

// --------------------------------------------------------
// Performs a bunch of arbitrary steps 
// to produce a deterministically random float3.
// --------------------------------------------------------
inline float3 random_float3(float3 pos, float offset)
{
	// 3x3 matrix of arbitrary values
	float3x3 m = float3x3(
		15.27f, 47.63f, 99.41f,
		89.98f, 95.07f, 38.39f,
		33.83f, 51.06f, 60.77f
	);
	
	// Do some arbitrary, random steps
	pos = frac(sin(mul(pos, m)) * 46839.32f);

	// Make a new float3, scaled by angle offset
	return float3(
		sin(pos.x * offset) * 0.5f + 0.5f,
		cos(pos.y * offset) * 0.5f + 0.5f,
		sin(pos.z * offset) * 0.5f + 0.5f
	);
}

// --------------------------------------------------------
// Random vector generation from Ch. 16 of Raytracing Gems.
// - Requires two uniformly distributed "random" float values.
// - Maps them to a sphere
// --------------------------------------------------------
float3 random_vector(float u0, float u1)
{
    float a = u0 * 2 - 1;
    float b = sqrt(1 - a * a);
    float phi = 2.0f * M_PI * u1;
	
    float x = b * cos(phi);
    float y = b * sin(phi);
    float z = a;
	
    return float3(x, y, z);
}

// --------------------------------------------------------
// Random vector in a hemisphere generation Raytracing Gems.
// - Requires two uniformly distributed "random" float values,
//   as well as a unit normal vector.
// --------------------------------------------------------
float3 random_cosine_weighted_hemisphere(
	float u0, float u1, float3 unitNormal)
{
    float a = u0 * 2 - 1;
    float b = sqrt(1 - a * a);
    float phi = 2.0f * M_PI * u1;
	
    float x = unitNormal.x + b * cos(phi);
    float y = unitNormal.y + b * sin(phi);
    float z = unitNormal.z + a;
    return float3(x, y, z);
}

// --------------------------------------------------------
// Does some stuff that's still a little over my head in
// terms of the actual Voronoi algorithm, but the result 
// is a 2D pattern of cells whose density and angle offset
// can be easily changed, and is based on the input's uv.
// --------------------------------------------------------
inline float Voronoi2D(float2 uv, float angle, float density)
{
	// Tile the space based on the cell density
	float2 g = floor(uv * density);
	float2 f = frac(uv * density);

	// Track the closest distance, used to color the pixel
	float minDist = 1.0f;
	
	// Loop through each neighboring tile in 2 dimensions
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			// Make a float3 for the neighbor cell we're looking at
			float2 neighbor = float2(x, y);
				
			// Get its deterministically random position,
			// using an angle offset
			float2 offset = random_float2(g + neighbor, angle);
				
			// Vector to that neighbor
			float2 diff = neighbor + offset - f;

			// Magnitude of that vector
			float dist = length(diff);
				
			// Keep the closer distance
			minDist = min(minDist, dist);
		}
	}
	
	return minDist;
}

// --------------------------------------------------------
// Does some stuff that's still a little over my head in
// terms of the actual Voronoi algorithm, but the result 
// is a 3D pattern of cells whose density and angle offset
// can be easily changed, and is based on the input's 
// world position.
// --------------------------------------------------------
inline float Voronoi3D(float3 pos, float angle, float density)
{
	// Tile the space based on the cell density
	float3 g = floor(pos * density);
	float3 f = frac(pos * density);

	// Track the closest distance, used to color the pixel
	float minDist = 1.0f;
	
	// Loop through each neighboring tile in 3 dimensions
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			for (int z = -1; z <= 1; z++)
			{
				// Make a float3 for the neighbor cell we're looking at
				float3 neighbor = float3(x, y, z);
				
				// Get its deterministically random position,
				// using an angle offset
				float3 offset = random_float3(g + neighbor, angle);
				
				// Vector to that neighbor
				float3 diff = neighbor + offset - f;

				// Magnitude of that vector
				float dist = length(diff);
				
				// Keep the closer distance
				minDist = min(minDist, dist);
			}
		}
	}
	
	return minDist;
}

#endif