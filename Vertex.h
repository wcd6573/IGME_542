/*
William Duprey
2/1/25
Vertex Definition
*/

#pragma once

#include <DirectXMath.h>

// --------------------------------------------------------
// A custom vertex definition.
// --------------------------------------------------------
struct Vertex
{
	DirectX::XMFLOAT3 Position;	    // Local position
	DirectX::XMFLOAT2 UV;			// Texture coordinate
	DirectX::XMFLOAT3 Normal;		// Normal vector from vertex
	DirectX::XMFLOAT3 Tangent;		// Tangent vector from vertex
};