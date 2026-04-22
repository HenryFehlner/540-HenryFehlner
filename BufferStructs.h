#pragma once

#include <DirectXMath.h>
#include "Lights.h"

struct VertexShaderData
{
	DirectX::XMFLOAT4X4 WorldMatrix;

	DirectX::XMFLOAT4X4 WorldInverseTransposeMatrix;

	DirectX::XMFLOAT4X4 ViewMatrix;

	DirectX::XMFLOAT4X4 ProjectionMatrix;

	DirectX::XMFLOAT4X4 LightViewMatrix;

	DirectX::XMFLOAT4X4 LightProjectionMatrix;
};

struct PixelShaderData
{
	DirectX::XMFLOAT2 UVScale;
	DirectX::XMFLOAT2 UVOffset;

	DirectX::XMFLOAT4 ColorTint;

	DirectX::XMFLOAT3 CameraPosition;
	float TotalTime;

	Light Lights[5];
};

struct SkyVertexShaderData
{
	DirectX::XMFLOAT4X4 ViewMatrix;

	DirectX::XMFLOAT4X4 ProjectionMatrix;
};