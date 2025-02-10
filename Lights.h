/*
William Duprey
10/24/24
Lights Header
*/

#pragma once
#include <DirectXMath.h>

// Must match definition in shaderes
#define MAX_LIGHTS 128

#define LIGHT_TYPE_DIRECTIONAL	0
#define LIGHT_TYPE_POINT		1
#define LIGHT_TYPE_SPOT			2

// --------------------------------------------------------
// Custom struct that can represent directional, point, or
// spot lights. Data oriented / padded to be in line with
// the 16-byte boundary.
// --------------------------------------------------------
struct Light {
	int Type;						// Which kind of light
	DirectX::XMFLOAT3 Direction;	// Directional / Spot need direction
	float Range;					// Point / Spot need max range for attenuation
	DirectX::XMFLOAT3 Position;		// Point / Spot need position in space
	float Intensity;				// All lights need intensity
	DirectX::XMFLOAT3 Color;		// All lights need color
	float SpotInnerAngle;			// Inner cone angle -- full light inside
	float SpotOuterAngle;			// Outer cone angle -- no light outside
	DirectX::XMFLOAT2 Padding;		// Pad to hit the 16-byte boundary
};