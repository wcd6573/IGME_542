/*
William Duprey
10/3/24
Camera Implementation
*/

#include "Camera.h"
#include "Input.h"
using namespace DirectX;

Camera::Camera(
    XMFLOAT3 startPosition,
    float _aspectRatio,
    float _fov,
    float _nearClip,
    float _farClip,
    bool _doPerspective,
    float _orthoWidth,
    float _moveSpeed,
    float _lookSpeed) :
    aspectRatio(_aspectRatio),
    fov(_fov),
    nearClip(_nearClip),
    farClip(_farClip),
    doPerspective(_doPerspective),
    orthoWidth(_orthoWidth),
    moveSpeed(_moveSpeed),
    lookSpeed(_lookSpeed)
{
    // Create transform and set starting position
    transform = std::make_shared<Transform>();
    transform->SetPosition(startPosition);

    // Calculate view and projection matrices
    UpdateViewMatrix();
    UpdateProjectionMatrix(aspectRatio);
}

// Shared pointers clean themselves up
Camera::~Camera() { }


///////////////////////////////////////////////////////////////////////////////
// ------------------------------- UPDATE ---------------------------------- //
///////////////////////////////////////////////////////////////////////////////
// --------------------------------------------------------
// Handles user input to move and rotate the camera.
// Updates the view matrix once any transformations happen.
// --------------------------------------------------------
void Camera::Update(float dt)
{
    // ----- Camera Position -----
    // Store movement so that there's 
    // only one call to MoveRelative for X and Z,
    // then one call to MoveAbsolute for Y
    XMFLOAT3 moveRel = XMFLOAT3(0, 0, 0);
    float moveAbs = 0.0f;

    // Speed modifier using the shift and control keys
    float speedMod = 1.0f;
    if (Input::KeyDown(VK_SHIFT)) { speedMod *= 2.0f; }
    if (Input::KeyDown(VK_CONTROL)) { speedMod /= 2.0f; }

    // No sense in redoing this calculation for each key
    float speed = moveSpeed * speedMod * dt;

    // Z-axis
    if (Input::KeyDown('W')) { moveRel.z += speed; }
    if (Input::KeyDown('S')) { moveRel.z += -speed; }
    // X-axis
    if (Input::KeyDown('D')) { moveRel.x += speed; }
    if (Input::KeyDown('A')) { moveRel.x += -speed; }
    // Y-axis
    if (Input::KeyDown(VK_SPACE)) { moveAbs += speed; }
    if (Input::KeyDown('X')) { moveAbs += -speed; }

    // Reposition the camera based on the user's input
    transform->MoveRelative(moveRel);
    transform->MoveAbsolute(0, moveAbs, 0);

    // ----- Camera Rotation -----
    // Only rotate the camera if the user clicks
    if (Input::MouseLeftDown())
    {
        // Store the amounts the mouse was moved since last frame
        float mouseX = Input::GetMouseXDelta() * lookSpeed;
        float mouseY = Input::GetMouseYDelta() * lookSpeed;

        // Rotate, mouseY = pitch, mouseX = yaw
        transform->Rotate(mouseY, mouseX, 0);

        // Clamp the pitch values
        // (There's probably a way to do this that doesn't
        // involve making a second call to the transform)
        XMFLOAT3 rotate = transform->GetRotation();
        float pitch = rotate.x;
        if (pitch < LOWER_LOOK_LIMIT)
        {
            rotate.x = LOWER_LOOK_LIMIT;
        }
        else if (pitch > UPPER_LOOK_LIMIT)
        {
            rotate.x = UPPER_LOOK_LIMIT;
        }
        transform->SetRotation(rotate);
    }

    // Update the view matrix last so that it matches
    // any changes that took place while updating
    UpdateViewMatrix();
}

// --------------------------------------------------------
// Calculates the camera's view matrix.
// Called when created, and once per frame, 
// as part of the camera Update method.
// --------------------------------------------------------
void Camera::UpdateViewMatrix()
{
    // These need to be variables and not just return values
    XMFLOAT3 pos = transform->GetPosition();
    XMFLOAT3 forward = transform->GetForward();

    // Create and save the view matrix
    XMStoreFloat4x4(&viewMatrix,
        XMMatrixLookToLH(
            XMLoadFloat3(&pos),         // Camera position
            XMLoadFloat3(&forward),     // Camera forward direction
            XMVectorSet(0, 1, 0, 0)));  // World up direction
}

// --------------------------------------------------------
// Calculates the camera's projection matrix.
// Called when created, and whenever the window is resized.
// --------------------------------------------------------
void Camera::UpdateProjectionMatrix(float _aspectRatio)
{
    // Set the new aspect ratio provided as the parameter
    aspectRatio = _aspectRatio;

    // Stores the result of the projection,
    // either perspective or orthographic
    XMMATRIX proj;

    // If doPerspective is true, do... perspective
    if (doPerspective)
    {
        proj = XMMatrixPerspectiveFovLH(
            fov, aspectRatio, nearClip, farClip);
    }
    // Otherwise, do orthographic
    else
    {
        // No need to store the view height, since it can
        // be derived from the view width and the aspect ratio
        proj = XMMatrixOrthographicLH(
            orthoWidth, orthoWidth / aspectRatio, nearClip, farClip);
    }

    // Store the result back in the projMatrix field
    XMStoreFloat4x4(&projMatrix, proj);
}


///////////////////////////////////////////////////////////////////////////////
// ------------------------------- GETTERS --------------------------------- //
///////////////////////////////////////////////////////////////////////////////
XMFLOAT4X4 Camera::GetViewMatrix() { return viewMatrix; }
XMFLOAT4X4 Camera::GetProjectionMatrix() { return projMatrix; }
std::shared_ptr<Transform> Camera::GetTransform() { return transform; }
float Camera::GetAspectRatio() { return aspectRatio; }
float Camera::GetFieldOfView() { return fov; }
float Camera::GetOrthographicWidth() { return orthoWidth; }
float Camera::GetNearClip() { return nearClip; }
float Camera::GetFarClip() { return farClip; }
float Camera::GetMoveSpeed() { return moveSpeed; }
float Camera::GetLookSpeed() { return lookSpeed; }
bool Camera::DoingPerspective() { return doPerspective; }


///////////////////////////////////////////////////////////////////////////////
// ------------------------------- SETTERS --------------------------------- //
///////////////////////////////////////////////////////////////////////////////
void Camera::SetMoveSpeed(float _moveSpeed) { moveSpeed = _moveSpeed; }
void Camera::SetLookSpeed(float _lookSpeed) { lookSpeed = _lookSpeed; }

// For all these methods that change the projection matrix,
// avoid recalculating projection matrix unnecessarily
void Camera::SetFieldOfView(float _fov)
{
    if (fov != _fov)
    {
        fov = _fov;
        UpdateProjectionMatrix(aspectRatio);
    }
}
void Camera::SetOrthographicWidth(float _orthoWidth)
{
    if (orthoWidth != _orthoWidth)
    {
        orthoWidth = _orthoWidth;
        UpdateProjectionMatrix(aspectRatio);
    }
}

void Camera::SetNearClip(float _nearClip)
{
    if (nearClip != _nearClip)
    {
        nearClip = _nearClip;
        UpdateProjectionMatrix(aspectRatio);
    }
}

void Camera::SetFarClip(float _farClip)
{
    if (farClip != _farClip)
    {
        farClip = _farClip;
        UpdateProjectionMatrix(aspectRatio);
    }
}

void Camera::SetPerspective(bool _doPerspective)
{
    if (doPerspective != _doPerspective)
    {
        doPerspective = _doPerspective;
        UpdateProjectionMatrix(aspectRatio);
    }
}
