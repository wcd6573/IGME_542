/*
William Duprey
10/3/24
Transform Class Header
*/

#pragma once
#include <DirectXMath.h>

// --------------------------------------------------------
// A class representing an entity's position, 
// rotation, and scale within the world.
// --------------------------------------------------------
class Transform
{
public:
	// No destructor since nothing on the heap
	Transform();

	// Getters
	DirectX::XMFLOAT3 GetPosition();
	DirectX::XMFLOAT3 GetRotation();
	DirectX::XMFLOAT3 GetScale();
	DirectX::XMFLOAT4X4 GetWorldMatrix();
	DirectX::XMFLOAT4X4 GetWorldInverseTransposeMatrix();
	DirectX::XMFLOAT3 GetRight();
	DirectX::XMFLOAT3 GetUp();
	DirectX::XMFLOAT3 GetForward();

	// Setters
	void SetPosition(float x, float y, float z);
	void SetPosition(DirectX::XMFLOAT3 _position);
	void SetRotation(float pitch, float yaw, float roll);
	void SetRotation(DirectX::XMFLOAT3 _rotation);
	void SetScale(float x, float y, float z);
	void SetScale(DirectX::XMFLOAT3 _scale);

	// Mutators
	void MoveAbsolute(float x, float y, float z);
	void MoveAbsolute(DirectX::XMFLOAT3 offset);
	void MoveRelative(float x, float y, float z);
	void MoveRelative(DirectX::XMFLOAT3 offset);
	void Rotate(float pitch, float yaw, float roll);
	void Rotate(DirectX::XMFLOAT3 _rotation);
	void Scale(float x, float y, float z);
	void Scale(DirectX::XMFLOAT3 _scale);

	// Helpers
	void UpdateWorld();
	void UpdateDirections();

private:
	// Vectors used to construct the world matrix
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 rotation;	// Could be a float4 quaternion? Scary
	DirectX::XMFLOAT3 scale;

	// Matrix fields, recalculated only when it is requested
	// and the above vectors have been updated
	DirectX::XMFLOAT4X4 worldMatrix;
	DirectX::XMFLOAT4X4 worldInverseTransposeMatrix;

	// Vector directions for the relative axes of the transform
	DirectX::XMFLOAT3 right;
	DirectX::XMFLOAT3 up;
	DirectX::XMFLOAT3 forward;

	// Whether the world matrix needs to be recalculated
	bool dirtyWorld;

	// Whether forward, up, and right need to be recalculated
	bool dirtyDirections;
};

