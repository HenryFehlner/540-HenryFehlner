#include "Material.h"
#include "Graphics.h"
#include <iostream>

Material::Material(
	DirectX::XMFLOAT2 pUVScale,
	DirectX::XMFLOAT2 pUVOffset,
	DirectX::XMFLOAT4 pColorTint, 
	Microsoft::WRL::ComPtr<ID3D11VertexShader> pVertexShader, 
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pPixelShader)
{
	UVScale = pUVScale;
	UVOffset = pUVOffset;
	colorTint = pColorTint;
	vertexShader = pVertexShader;
	pixelShader = pPixelShader;

	srvCount = 0;
	samplerCount = 0;
}

void Material::AddTextureSRV(unsigned int slot, ID3D11ShaderResourceView* srv)
{
	textureSRVs[slot] = srv;
	srvCount++;
}
void Material::AddSampler(unsigned int slot, ID3D11SamplerState* samplerState)
{
	samplers[slot] = samplerState;
	samplerCount++;
}
void Material::BindTexturesAndSamplers()
{
	for (unsigned int i = 0; i < srvCount; ++i)
	{
		Graphics::Context->PSSetShaderResources(i, 1, textureSRVs[i].GetAddressOf());
	}

	for (unsigned int i = 0; i < samplerCount; ++i)
	{
		Graphics::Context->PSSetSamplers(i, 1, samplers[i].GetAddressOf());
	}
}

// Setters
void Material::SetUVScale(DirectX::XMFLOAT2 pUVScale)
{
	UVScale = pUVScale;
}
void Material::SetUVOffset(DirectX::XMFLOAT2 pUVOffset)
{
	UVOffset = pUVOffset;
}
void Material::SetColorTint(DirectX::XMFLOAT4 pColorTint)
{
	colorTint = pColorTint;
}
void Material::SetVertexShader(Microsoft::WRL::ComPtr<ID3D11VertexShader> pVertexShader)
{
	vertexShader = pVertexShader;
}
void Material::SetPixelShader(Microsoft::WRL::ComPtr<ID3D11PixelShader> pPixelShader)
{
	pixelShader = pPixelShader;
}

// Getters
DirectX::XMFLOAT2 Material::GetUVScale()
{
	return UVScale;
}
DirectX::XMFLOAT2 Material::GetUVOffset()
{
	return UVOffset;
}
DirectX::XMFLOAT4 Material::GetColorTint()
{
	return colorTint;
}
unsigned int Material::GetTextureCount()
{
	return srvCount;
}
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* Material::GetTextureSRVs()
{
	return textureSRVs;
}
Microsoft::WRL::ComPtr<ID3D11VertexShader> Material::GetVertexShader()
{
	return vertexShader;
}
Microsoft::WRL::ComPtr<ID3D11PixelShader> Material::GetPixelShader()
{
	return pixelShader;
}
