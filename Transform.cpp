#include "Transform.h"

using namespace DirectX;	// for overload operators

Transform::Transform() : 
	position(0, 0, 0),		// This is technically faster
	rotation(0, 0, 0), 
	scale(1, 1, 1),
	matrixDirty(false)
{
	// Initialize values
	XMStoreFloat4x4(&worldMatrix, DirectX::XMMatrixIdentity());
	XMStoreFloat4x4(&worldInverseTransposeMatrix, DirectX::XMMatrixIdentity());
}

Transform::Transform(float initialX, float initialY, float initialZ) :
	position(initialX, initialY, initialZ),
	rotation(0, 0, 0),
	scale(1, 1, 1),
	matrixDirty(false)
{
	// Initialize values
	XMStoreFloat4x4(&worldMatrix, DirectX::XMMatrixIdentity());
	XMStoreFloat4x4(&worldInverseTransposeMatrix, DirectX::XMMatrixIdentity());
}

Transform::~Transform()
{

}

// Rebuilds the world matrix, meant to 
// be called in the GetMatrix() method
void Transform::UpdateWorldMatrix()
{
	// Create separate matrices
	XMMATRIX positionMat = XMMatrixTranslationFromVector(XMLoadFloat3(&position));
	XMMATRIX rotationMat = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&rotation));
	XMMATRIX scaleMat = XMMatrixScalingFromVector(XMLoadFloat3(&scale));

	// Create world matrix
	XMMATRIX worldMat = scaleMat * rotationMat * positionMat;

	// Store world matrix to a variable
	XMStoreFloat4x4(&worldMatrix, worldMat);
	XMStoreFloat4x4(&worldInverseTransposeMatrix, XMMatrixInverse(0, XMMatrixTranspose(worldMat)));
}

// Setters
void Transform::SetPosition(float x, float y, float z)
{
	position.x = x;
	position.y = y;
	position.z = z;

	matrixDirty = true;
}
void Transform::SetPosition(DirectX::XMFLOAT3 pPosition)
{
	position = pPosition;

	matrixDirty = true;
}
void Transform::SetRotation(float pitch, float yaw, float roll)
{
	rotation.x = pitch;
	rotation.y = yaw;
	rotation.z = roll;

	matrixDirty = true;
}
void Transform::SetRotation(DirectX::XMFLOAT3 pRotation)
{
	rotation = pRotation;

	matrixDirty = true;
}
void Transform::SetScale(float x, float y, float z)
{
	scale.x = x;
	scale.y = y;
	scale.z = z;

	matrixDirty = true;
}
void Transform::SetScale(DirectX::XMFLOAT3 pScale)
{
	scale = pScale;

	matrixDirty = true;
}

// Getters
DirectX::XMFLOAT3 Transform::GetPosition()
{
	return position;
}
DirectX::XMFLOAT3 Transform::GetPitchYawRoll()
{
	return rotation;
}
DirectX::XMFLOAT3 Transform::GetScale()
{
	return scale;
}
DirectX::XMFLOAT4X4 Transform::GetWorldMatrix()
{
	// Update matrix if dirty
	if (matrixDirty)
	{
		UpdateWorldMatrix();

		matrixDirty = false;
	}
	return worldMatrix;
}
DirectX::XMFLOAT4X4 Transform::GetWorldInverseTransposeMatrix()
{
	return worldInverseTransposeMatrix;
}
DirectX::XMFLOAT3 Transform::GetRight()
{
	// Load data into math types
	XMFLOAT3 rightward(1.0f, 0.0f, 0.0f);
	XMVECTOR rightVec = XMLoadFloat3(&rightward);

	// Create and load quaternion from transform's current rotation
	XMVECTOR currentRotation = XMQuaternionRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);

	// Rotate by this rotation
	rightVec = XMVector3Rotate(rightVec, currentRotation);

	// Back to storage type
	XMStoreFloat3(&rightward, rightVec);

	return rightward;
}
DirectX::XMFLOAT3 Transform::GetUp()
{
	// Load data into math types
	XMFLOAT3 upward(0.0f, 1.0f, 0.0f);
	XMVECTOR upVec = XMLoadFloat3(&upward);

	// Create and load quaternion from transform's current rotation
	XMVECTOR currentRotation = XMQuaternionRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);

	// Rotate by this rotation
	upVec = XMVector3Rotate(upVec, currentRotation);

	// Back to storage type
	XMStoreFloat3(&upward, upVec);

	return upward;
}
DirectX::XMFLOAT3 Transform::GetForward()
{
	// Load data into math types
	XMFLOAT3 forward(0.0f, 0.0f, 1.0f);
	XMVECTOR forwardVec = XMLoadFloat3(&forward);

	// Create and load quaternion from transform's current rotation
	XMVECTOR currentRotation = XMQuaternionRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);

	// Rotate by this rotation
	forwardVec = XMVector3Rotate(forwardVec, currentRotation);

	// Back to storage type
	XMStoreFloat3(&forward, forwardVec);

	return forward;
}

// Transformers (more than meets the eye)
void Transform::MoveAbsolute(float x, float y, float z)
{
	// Call the other version
	XMFLOAT3 offset(x, y, z);
	MoveAbsolute(offset);
}
void Transform::MoveAbsolute(DirectX::XMFLOAT3 pOffset)
{
	// Load data into math types
	XMVECTOR posVec = XMLoadFloat3(&position);
	XMVECTOR offsetVec = XMLoadFloat3(&pOffset);

	// Add to position
	posVec += offsetVec;

	// Back to storage type
	XMStoreFloat3(&position, posVec);

	matrixDirty = true;
}
void Transform::MoveRelative(float x, float y, float z)
{
	// Call the other version
	XMFLOAT3 offset(x, y, z);
	MoveRelative(offset);
}
void Transform::MoveRelative(DirectX::XMFLOAT3 pOffset)
{
	// Load data into math types
	XMVECTOR posVec = XMLoadFloat3(&position);
	XMVECTOR offsetVec = XMLoadFloat3(&pOffset);

	// Create and load quaternion from transform's current rotation
	XMVECTOR currentRotation = XMQuaternionRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);

	// Do tha math
	posVec += XMVector3Rotate(offsetVec, currentRotation);

	// Back to storage type
	XMStoreFloat3(&position, posVec);

	matrixDirty = true;
}
void Transform::Rotate(float pitch, float yaw, float roll)
{
	// Call the other version
	XMFLOAT3 rotationOffset(pitch, yaw, roll);
	Rotate(rotationOffset);
}
void Transform::Rotate(DirectX::XMFLOAT3 pRotation)
{
	// Load data into math types
	XMVECTOR rotVec = XMLoadFloat3(&rotation);
	XMVECTOR rotationOffsetVec = XMLoadFloat3(&pRotation);

	// Do the math
	rotVec += rotationOffsetVec;

	// Back to storage type
	XMStoreFloat3(&rotation, rotVec);

	matrixDirty = true;
}
void Transform::Scale(float pScale)
{
	// Call the other version
	XMFLOAT3 scaleOffset(pScale, pScale, pScale);
	Scale(scaleOffset);
}
void Transform::Scale(float x, float y, float z)
{
	// Call the other version
	XMFLOAT3 scaleOffset(x, y, z);
	Scale(scaleOffset);
}
void Transform::Scale(DirectX::XMFLOAT3 pScale)
{
	// Load data into math types
	XMVECTOR scVec = XMLoadFloat3(&scale);
	XMVECTOR scOffsetVec = XMLoadFloat3(&pScale);

	// Do the math
	scVec *= scOffsetVec;

	// Back to storage type
	XMStoreFloat3(&scale, scVec);

	matrixDirty = true;
}