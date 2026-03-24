#pragma once

#include <d3d11.h>
#include <d3d11shadertracing.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include "Entity.h"
#include "Mesh.h"
#include "Camera.h"
#include "Material.h"

class Game
{
public:
	// Basic OOP setup
	Game();
	~Game();
	Game(const Game&) = delete; // Remove copy constructor
	Game& operator=(const Game&) = delete; // Remove copy-assignment operator

	// Primary functions
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);
	void OnResize();

private:

	// Initialization helper methods - feel free to customize, combine, remove, etc.
	//void LoadShaders();
	Microsoft::WRL::ComPtr<ID3D11VertexShader> LoadVertexShader(std::wstring shaderPath);
	Microsoft::WRL::ComPtr<ID3D11PixelShader> LoadPixelShader(std::wstring shaderPath);
	Microsoft::WRL::ComPtr<ID3D11InputLayout> LoadInputLayout(ID3DBlob* vertexShaderBlob);

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	//// Buffers to hold actual geometry data
	//Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	//Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

	// Shaders and shader-related constructs
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;

	Microsoft::WRL::ComPtr<ID3D11PixelShader> tintPixelShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> debugUVsPixelShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> debugNormalsPixelShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> customPixelShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> combinedPixelShader;

	// ImGui update helper
	void ImGuiNewFrameUpdate(float deltaTime);
	void ImGuiBuildUI();

	// Constant ring buffer
	Microsoft::WRL::ComPtr<ID3D11DeviceContext1> Context1;	// D3D11.1 context
	Microsoft::WRL::ComPtr<ID3D11Buffer> ConstantBufferHeap;	// Ring buffer
	unsigned int cbHeapSizeInBytes;
	unsigned int cbHeapOffsetInBytes;
	void FillAndBindNextConstantBuffer(
		void* data,
		unsigned int dataSizeInBytes,
		D3D11_SHADER_TYPE shaderType,
		unsigned int registerSlot
	);

	// Material container
	std::vector<std::shared_ptr<Material>> materialVec;

	// Mesh container
	std::vector<std::shared_ptr<Mesh>> meshVec;

	// Entity container
	std::vector<std::shared_ptr<Entity>> entityVec;

	// Camera container
	std::vector<std::shared_ptr<Camera>> cameraVec;
	unsigned int activeCameraIndex;
	std::shared_ptr<Camera> camera1;
	std::shared_ptr<Camera> camera2;

	// Texture sampler
	Microsoft::WRL::ComPtr<ID3D11SamplerState> texSampler;

	// Textures
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pavingStonesSrv;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> graffitiSrv;

	// Materials
	std::shared_ptr<Material> tintPsMaterial;
	std::shared_ptr<Material> uvPsMaterial;
	std::shared_ptr<Material> normalsPsMaterial;
	std::shared_ptr<Material> customPsMaterial;
	std::shared_ptr<Material> combinedPsMaterial;

	// Meshes
	std::shared_ptr<Mesh> cubeMesh;
	std::shared_ptr<Mesh> cylinderMesh;
	std::shared_ptr<Mesh> helixMesh;
	std::shared_ptr<Mesh> sphereMesh;
	std::shared_ptr<Mesh> torusMesh;
	std::shared_ptr<Mesh> quadMesh;
	std::shared_ptr<Mesh> quadDoubleMesh;

	// Entities
	std::shared_ptr<Entity> testEntity1;
	std::shared_ptr<Entity> testEntity2;
	std::shared_ptr<Entity> testEntity3;
	std::shared_ptr<Entity> testEntity4;
	std::shared_ptr<Entity> testEntity5;
};

