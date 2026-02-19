#pragma once

#include <DirectXMath.h>

struct VertexShaderData
{
	DirectX::XMFLOAT4 ColorTint;
	DirectX::XMFLOAT4X4 WorldMatrix;
	DirectX::XMFLOAT4X4 ViewMatrix;
	DirectX::XMFLOAT4X4 ProjectionMatrix;
};