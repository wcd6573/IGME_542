/*
William Duprey
2/2/25
Buffer Structs Definitions
*/

#pragma once
#include <DirectXMath.h>

// --------------------------------------------------------
// Struct that maps exactly to the vertex shader's cbuffer.
// --------------------------------------------------------
struct VertexShaderExternalData {
	DirectX::XMFLOAT4X4 World;
	DirectX::XMFLOAT4X4 WorldInvTrans;
	DirectX::XMFLOAT4X4 View;
	DirectX::XMFLOAT4X4 Projection;
};