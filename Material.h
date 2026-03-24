#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <WICTextureLoader.h>

class Material
{
public:
	Material(
		DirectX::XMFLOAT2 pUVScale,
		DirectX::XMFLOAT2 pUVOffset,
		DirectX::XMFLOAT4 pColorTint, 
		Microsoft::WRL::ComPtr<ID3D11VertexShader> pVertexShader, 
		Microsoft::WRL::ComPtr<ID3D11PixelShader> pPixelShader);

	void AddTextureSRV(unsigned int slot, ID3D11ShaderResourceView* srv);
	void AddSampler(unsigned int slot, ID3D11SamplerState* samplerState);
	void BindTexturesAndSamplers();

	// Setters
	void SetUVScale(DirectX::XMFLOAT2 pUVScale);
	void SetUVOffset(DirectX::XMFLOAT2 pUVOffset);
	void SetColorTint(DirectX::XMFLOAT4 pColorTint);
	void SetVertexShader(Microsoft::WRL::ComPtr<ID3D11VertexShader> pVertexShader);
	void SetPixelShader(Microsoft::WRL::ComPtr<ID3D11PixelShader> pPixelShader);

	// Getters
	DirectX::XMFLOAT2 GetUVScale();
	DirectX::XMFLOAT2 GetUVOffset();
	DirectX::XMFLOAT4 GetColorTint();
	unsigned int GetTextureCount();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* GetTextureSRVs();
	Microsoft::WRL::ComPtr<ID3D11VertexShader> GetVertexShader();
	Microsoft::WRL::ComPtr<ID3D11PixelShader> GetPixelShader();

private:
	DirectX::XMFLOAT2 UVScale;
	DirectX::XMFLOAT2 UVOffset;
	DirectX::XMFLOAT4 colorTint;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;

	// Arrays of SRVs and Sampler States
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureSRVs[128];
	unsigned int srvCount;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplers[16];
	unsigned int samplerCount;
};