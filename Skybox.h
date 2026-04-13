#pragma once
#include <d3d11.h>
#include <memory>
#include <wrl/client.h>
#include "Mesh.h"
#include "Camera.h"

class Skybox
{
public:
	Skybox(
		std::shared_ptr<Mesh> pMesh, 
		ID3D11SamplerState* pSamplerState, 
		ID3D11VertexShader* pVertexShader, 
		ID3D11PixelShader* pPixelShader,
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back);

	void Draw(float deltaTime);

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateCubemap(
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back);

private:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	std::shared_ptr<Mesh> meshPtr;
};