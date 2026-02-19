#include "Camera.h"
#include "Input.h"
#include <algorithm>	// For clamp method

using namespace DirectX;	// for overload operators

Camera::Camera(float xPos, float yPos, float zPos, 
	float pAspectRatio, float pFovAngle, 
	float pNearClip, float pFarClip, 
	float pMoveSpeed, float pLookSpeed, 
	bool pIsPerspective)
{
	// Initialize variables
	transform = Transform(xPos, yPos, zPos);
	fovAngle = pFovAngle;
	nearClipDist = pNearClip;
	farClipDist = pFarClip;
	movementSpeed = pMoveSpeed;
	mouseLookSpeed = pLookSpeed;
	isPerspective = pIsPerspective;

	// Matrix creation
	UpdateViewMatrix();
	UpdateProjectionMatrix(pAspectRatio);
}

Camera::~Camera()
{
}

void Camera::UpdateViewMatrix()
{
	// Load into math types
	XMFLOAT3 worldUp(0.0f, 1.0f, 0.0f);
	XMVECTOR worldUpVec = XMLoadFloat3(&worldUp);
	XMFLOAT3 position = transform.GetPosition();
	XMVECTOR posVec = XMLoadFloat3(&position);
	XMFLOAT3 direction = transform.GetForward();
	XMVECTOR directionVec = XMLoadFloat3(&direction);

	// Create view matrix
	XMMATRIX viewMat = XMMatrixLookToLH(posVec, directionVec, worldUpVec);

	// Back to storage
	XMStoreFloat4x4(&viewMatrix, viewMat);
}

void Camera::UpdateProjectionMatrix(float pAspectRatio)
{
	if (isPerspective)
	{
		// Create new projection matrix
		XMMATRIX projMat = XMMatrixPerspectiveFovLH(fovAngle, pAspectRatio, nearClipDist, farClipDist);

		// Store the new matrix
		XMStoreFloat4x4(&projMatrix, projMat);
	}
	else
	{
		// Orthographic view not yet implemented
	}
}

void Camera::Update(float deltaTime)
{
	// Keyboard input handling
	if (Input::KeyDown('W'))			{ transform.MoveRelative(0.0f, 0.0f, 1.0f * deltaTime * movementSpeed); }
	else if (Input::KeyDown('S'))		{ transform.MoveRelative(0.0f, 0.0f, -1.0f * deltaTime * movementSpeed); }
	if (Input::KeyDown('A'))			{ transform.MoveRelative(-1.0f * deltaTime * movementSpeed, 0.0f, 0.0f); }
	else if (Input::KeyDown('D'))		{ transform.MoveRelative(1.0f * deltaTime * movementSpeed, 0.0f, 0.0f); }
	if (Input::KeyDown(VK_SPACE))		{ transform.MoveAbsolute(0.0f, 1.0f * deltaTime * movementSpeed, 0.0f); }
	else if (Input::KeyDown(VK_SHIFT))	{ transform.MoveAbsolute(0.0f, -1.0f * deltaTime * movementSpeed, 0.0f); }

	// Mouse input handling
	if (Input::MouseLeftDown())
	{
		// Get mouse input
		float cursorMoveX = Input::GetMouseXDelta() * mouseLookSpeed;
		float cursorMoveY = Input::GetMouseYDelta() * mouseLookSpeed;

		// Rotate camera
		transform.Rotate(cursorMoveY, cursorMoveX, 0.0f);	// Clamp pitch
		
		// Clamp the pitch (this is a dumb way to do it probably)
		XMFLOAT3 rotation = transform.GetPitchYawRoll();
		rotation.x = std::clamp(rotation.x, -1.57f, 1.57f);
		transform.SetRotation(rotation);
	}

	// Update view matrix
	UpdateViewMatrix();
}

DirectX::XMFLOAT4X4 Camera::GetViewMatrix()
{
	return viewMatrix;
}
DirectX::XMFLOAT4X4 Camera::GetProjectionMatrix()
{
	return projMatrix;
}
DirectX::XMFLOAT3 Camera::GetPosition()
{
	return transform.GetPosition();
}
float Camera::GetFov()
{
	return fovAngle;
}