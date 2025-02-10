/*
William Duprey
2/2/25
Buffer Structs Definitions
*/

#pragma once
#include <DirectXMath.h>
#include "Lights.h"

// --------------------------------------------------------
// Struct that maps exactly to the vertex shader's cbuffer.
// --------------------------------------------------------
struct VertexShaderExternalData 
{
	DirectX::XMFLOAT4X4 World;
	DirectX::XMFLOAT4X4 WorldInvTrans;
	DirectX::XMFLOAT4X4 View;
	DirectX::XMFLOAT4X4 Projection;
};

// --------------------------------------------------------
// Struct that maps exactly to the pixel shader's cbuffer.
// --------------------------------------------------------
struct PixelShaderExternalData 
{
	DirectX::XMFLOAT2 uvScale;
	DirectX::XMFLOAT2 uvOffset;
	DirectX::XMFLOAT3 cameraPosition;
	int lightCount;
	Light lights[MAX_LIGHTS];
};