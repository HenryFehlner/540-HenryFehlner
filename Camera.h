#pragma once
#include <DirectXMath.h>
#include "Transform.h"

class Camera
{
public:
	Camera(float xPos, float yPos, float zPos, 
		float pAspectRatio, float pFovAngle, 
		float pNearClip, float pFarClip, 
		float pMoveSpeed, float pLookSpeed,
		bool pIsPerspective);
	~Camera();

	void UpdateViewMatrix();
	void UpdateProjectionMatrix(float pAspectRatio);

	void Update(float deltaTime);

	// Getters
	DirectX::XMFLOAT4X4 GetViewMatrix();
	DirectX::XMFLOAT4X4 GetProjectionMatrix();
	DirectX::XMFLOAT3 GetPosition();
	float GetFov();

private:
	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 projMatrix;
	Transform transform;
	float fovAngle;	// radians
	float nearClipDist;
	float farClipDist;
	float movementSpeed;
	float mouseLookSpeed;
	bool isPerspective;
};