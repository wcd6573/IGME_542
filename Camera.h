/*
William Duprey
10/3/24
Camera Header
*/

#pragma once
#include <DirectXMath.h>
#include <memory>
#include "Transform.h"

// --------------------------------------------------------
// A class representing a camera. It contains view
// and projection matrices, as well as other fields
// for controlling how the scene is viewed.
// --------------------------------------------------------
class Camera
{
public:
	// Camera takes a start position so that
	// it doesn't always start at the origin
	Camera(DirectX::XMFLOAT3 startPosition,
		float _aspectRatio,
		float _fov = DirectX::XM_PIDIV4,// Give default values
		float _nearClip = 0.01f,		// for some fields 
		float _farClip = 100.0f,
		bool _doPerspective = true,
		float _orthoWidth = 10.0f,
		float _moveSpeed = 5.0f,
		float _lookSpeed = 0.002f);
	~Camera();

	// Update methods
	void Update(float dt);
	void UpdateViewMatrix();
	void UpdateProjectionMatrix(float _aspectRatio);

	// Getters
	DirectX::XMFLOAT4X4 GetViewMatrix();
	DirectX::XMFLOAT4X4 GetProjectionMatrix();
	std::shared_ptr<Transform> GetTransform();
	float GetAspectRatio();
	float GetFieldOfView();
	float GetOrthographicWidth();
	float GetNearClip();
	float GetFarClip();
	float GetMoveSpeed();
	float GetLookSpeed();
	bool DoingPerspective();

	// Setters
	void SetFieldOfView(float _fov);
	void SetOrthographicWidth(float _orthoWidth);
	void SetNearClip(float _nearClip);
	void SetFarClip(float _farClip);
	void SetMoveSpeed(float _moveSpeed);
	void SetLookSpeed(float _lookSpeed);
	void SetPerspective(bool _doPerspective);

private:
	// Basic fields
	std::shared_ptr<Transform> transform;
	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 projMatrix;
	float aspectRatio;

	// Extra customization
	float fov;		// in radians
	float nearClip;	// near clip plane distance
	float farClip;	// far clip plane distance
	float moveSpeed;	// How fast you move around
	float lookSpeed;	// How fast you can rotate the camera
	bool doPerspective;	// perspective, or orthographic projection
	float orthoWidth;	// Width of orthographic view

	// Constants for constraining the min and max pitch values
	const float UPPER_LOOK_LIMIT = DirectX::XM_PIDIV2 - 0.05f;
	const float LOWER_LOOK_LIMIT = -DirectX::XM_PIDIV2 + 0.05f;
};
