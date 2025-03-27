/*
William Duprey
3/5/25
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

// --------------------------------------------------------
// Overall scene data for raytracing.
// --------------------------------------------------------
struct RaytracingSceneData 
{
	DirectX::XMFLOAT4X4 inverseViewProjection;
	DirectX::XMFLOAT3 cameraPosition;
	float pad;
};

// Ensure this matches the Raytracing shader define
#define MAX_INSTANCES_PER_BLAS 100

// --------------------------------------------------------
// Struct that stores an array of colors, with an entry
// for each BLAS, which the shader code will access using
// the instanceID defined when the BLAS is made.
// --------------------------------------------------------
struct RaytracingEntityData 
{
	DirectX::XMFLOAT4 color[MAX_INSTANCES_PER_BLAS];
	float refraction[MAX_INSTANCES_PER_BLAS];
};