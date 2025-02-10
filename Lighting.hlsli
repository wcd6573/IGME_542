/*
William Duprey
11/23/24
Shader Lighting Header
*/

#ifndef __GGP_LIGHTING__ 
#define __GGP_LIGHTING__

////////////////////////////////////////////////////////////////////////////////
// -------------------------------- CONSTANTS ------------------------------- //
////////////////////////////////////////////////////////////////////////////////
#define MAX_SPECULAR_EXPONENT 256.0f  
#define LIGHT_TYPE_DIRECTIONAL	0
#define LIGHT_TYPE_POINT		1
#define LIGHT_TYPE_SPOT			2

// Use 'static const' or '#define', what's the difference?
// Constant Fresnel value for non-metals
// (like glass and plastic)
static const float F0_NON_METAL = 0.04f;

// Minimum roughness for when spec distribution 
// function denominator goes to 0
static const float MIN_ROUGHNESS = 0.0000001f;

// Gotta have pi, that's just a given
static const float PI = 3.14159265359f;

////////////////////////////////////////////////////////////////////////////////
// --------------------------------- STRUCTS -------------------------------- //
////////////////////////////////////////////////////////////////////////////////
// --------------------------------------------------------
// Custom struct that can represent directional, point, or
// spot lights. Data oriented / padded to be in line with
// the 16-byte boundary.
// --------------------------------------------------------
struct Light
{
    int Type; // Which kind of light
    float3 Direction; // Directional / Spot need direction
    float Range; // Point / Spot need max range for attenuation
    float3 Position; // Point / Spot need position in space
    float Intensity; // All lights need intensity
    float3 Color; // All lights need color
    float SpotInnerAngle; // Inner cone angle -- full light inside
    float SpotOuterAngle; // Outer cone angle -- no light outside
    float2 Padding; // Pad to hit the 16-byte boundary
};


////////////////////////////////////////////////////////////////////////////////
// --------------------------- PBR CALCULATIONS ----------------------------- //
////////////////////////////////////////////////////////////////////////////////
// PBR function code provided by Prof. Chris Cascioli

// Lambert diffuse BRDF - Same as the basic lighting diffuse calculation
// - NOTE: this function assumes the vectors are already normalized
float DiffusePBR(float3 normal, float3 dirToLight)
{
    return saturate(dot(normal, dirToLight));
}

// Calculates diffuse amount based on energy conservation
// diffuse      - Diffuse amount
// F            - Fresnel result from microfacet BRDF
// metalness    - surface metalness amount
float3 DiffuseEnergyConserve(float3 diffuse, float3 F, float metalness)
{
    return diffuse * (1 - F) * (1 - metalness);
}

// Normal Distribution Function: GGX (Trowbridge-Reitz)
// a - Roughness
// h - half vector
// n - normal
// D(h, n, a) = a^2 / pi * ((n dot h)^2 * (a^2 - 1) + 1)^2
float D_GGX(float3 n, float3 h, float roughness)
{
    // Pre-calculations
    float NdotH = saturate(dot(n, h));
    float NdotH2 = NdotH * NdotH;
    float a = roughness * roughness;
    float a2 = max(a * a, MIN_ROUGHNESS); // Applied after remap
    
    // ((n dot h)^2 * (a^2 - 1) + 1)^2
    // Can go to zero if roughness is 0 and NdotH is 1
    float denomToSquare = NdotH2 * (a2 - 1) + 1;
    
    // Final value
    return a2 / (PI * denomToSquare * denomToSquare);
}

// Fresnel term - Schlick approx.
// v  - View vector
// h  - Half vector
// f0 - Value when 1 = n
// F(v, h, f0) = f0 + (1 - f0)(1 - (v dot h))^5
float3 F_Schlick(float3 v, float3 h, float3 f0)
{
    // Pre-calculations
    float VdotH = saturate(dot(v, h));
    
    // Final value
    return f0 + (1 - f0) * pow(1 - VdotH, 5);
}

// Geometric Shadowing - Schlick-GGX
// - NOTE: k is remapped to a / 2, 
//         roughness remapped to (r+1)/2 before squaring
// n - Normal
// v - view vector
//
// G_Schlick(n, v, a) = (n dot v) / ((n dot v) * (1 - k) * k)
// Full G(n, v, l, a) term = G_SchlickGGX(n, v, a) * G_SchlickGGX(n, l, a)
float G_SchlickGGX(float3 n, float3 v, float roughness)
{
    // End result of remapping:
    float k = pow(roughness + 1, 2) / 8.0f;
    float NdotV = saturate(dot(n, v));
    
    // Final value
    // Note: Numerator should be NdotV (or NdotL depending on params)
    //  However, these are also in the BRDF's denominator, so they'll cancel.
    //  We're leaving them out here AND in the BRDF function as the
    //  dot products can get VERY small and cause rounding errors.
    return 1 / (NdotV * (1 - k) + k);
}

// Cook-Torrance Microfacet BRDF (Specular)
// f(l, v) = D(h)F(v, h)G(l, v, h) / 4(n dot l)(n dot v)
// - parts of the denominator are canceled out by numerator (see below)
//
// D() - Normal Distribution Function - Trowbridge-Reitz (GGX)
// F() - Fresnel - Schlick approx
// G() - Geometric Shadowing - Schlick-GGX
float3 MicrofacetBRDF(float3 n, float3 l, float3 v,
    float roughness, float3 f0, out float3 F_out)
{
    // Other vectors
    float3 h = normalize(v + l);
    
    // Run numerator functions
    float D = D_GGX(n, h, roughness);
    float3 F = F_Schlick(v, h, f0);
    float G = G_SchlickGGX(n, v, roughness) *
        G_SchlickGGX(n, l, roughness);

    // Pass F out of the function for diffuse balance
    F_out = F;
    
    // Final specular formula
    // Note: The denominator SHOULD contain (NdotV)(NdotL), but they'd be
    //  canceled out by our G() term. As such, they have been removed
    //  from BOTH places to prevent floating point rounding errors.
    float3 specularResult = (D * F * G) / 4;
    
    // One last non-obvious requirement: According to the rendering equation,
    // specular must have the same NdotL applied as diffuse. We'll apply
    // that here so that minimal changes are required elsewhere.
    return specularResult * max(dot(n, l), 0);
}

// Calculates light using the physically-based rendering model.
float3 CalculateLightPBR(float3 lightColor, float lightIntensity,
    float3 normal, float3 toLight, float3 toCam, float3 surfaceColor,
    float3 specColor, float roughness, float metalness)
{
    // Calculate diffuse and specular light
    float diff = DiffusePBR(normal, toLight);
    float3 F; // Used to store Fresnel from specular calculation
    float3 spec = MicrofacetBRDF(normal, toLight, toCam, roughness, specColor, F);

    // Conserve energy and cut diffuse for metals
    float3 balancedDiff = DiffuseEnergyConserve(diff, F, metalness);
    
    // Combine final diffuse and specular values, and return
    return (balancedDiff * surfaceColor + spec) * lightIntensity * lightColor;
}


////////////////////////////////////////////////////////////////////////////////
// -------------------------- NON-PBR FUNCTIONS ------------------------------- //
////////////////////////////////////////////////////////////////////////////////
float3 calculateDiffuse(float3 normal, float3 toLight)
{
    return saturate(dot(normal, toLight));
}

float3 calculateSpecular(float3 normal, float3 toLight,
    float3 toCam, float roughness)
{
    // Calculate perfect reflection vector
    float3 reflVec = reflect(-toLight, normal);
    float specExp = (1.0f - roughness) * MAX_SPECULAR_EXPONENT;

    
    // Return final calculated light
    return pow(saturate(dot(toCam, reflVec)), specExp);
}

// Attenuation function with non-linear falloff, and
// returns 0 when outside the light's range
float attenuate(Light light, float3 worldPos)
{
    float dist = distance(light.Position, worldPos);
    float att = saturate(1.0f - (dist * dist / (light.Range * light.Range)));
    return att * att;
}

float3 directionalLightOld(Light light, float3 color, float3 normal,
    float3 cameraPos, float3 worldPos, float roughness, float specScale)
{
    // --- Diffuse ---
    float3 toLight = normalize(-light.Direction);
    float3 diffuse = calculateDiffuse(normal, toLight);
    
    // --- Specular ---
    // Calculate vector from camera to pixel
    float3 toCam = normalize(cameraPos - worldPos);
    float3 specular = calculateSpecular(normal, toLight, toCam, roughness)
        * specScale; // Scale by specular map value
    
    // Comments copied verbatim from assignment document:
    // Cut the specular if the diffuse contribution is zero
    // - any() returns 1 if any component of the param is non-zero
    // - In other words:
    //     - If the diffuse amount is 0, any(diffuse) returns 0
    //     - If the diffuse amount is != 0, any(diffuse) returns 1
    //     - So when diffuse is 0, specular becomes 0
    specular *= any(diffuse);
    
    // Combine light and return
    // Multiply specular by color? No right answer
    return ((diffuse * color) + specular) *
        light.Intensity * light.Color;
}

float3 pointLightOld(Light light, float3 color, float3 normal,
    float3 cameraPos, float3 worldPos, float roughness, float specScale)
{
    // --- Diffuse ---
    // Point lights have no direction, so calculate one
    // based on the world position and light's position
    float3 toLight = normalize(light.Position - worldPos);
    float3 diffuse = calculateDiffuse(normal, toLight);
    
    // --- Specular ---
    float3 toCam = normalize(cameraPos - worldPos);
    float3 specular = calculateSpecular(normal, toLight, toCam, roughness)
        * specScale; // Scale by specular map value

    // Combine light, scale by attenuation, and return
    return ((diffuse * color) + specular) *
        light.Intensity * light.Color * attenuate(light, worldPos);
}

#endif