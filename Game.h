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
#include "Lights.h"
#include "Skybox.h"

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
	// Global lighting data
	Light lights[5];

	// Initialization helper methods - feel free to customize, combine, remove, etc.
	//void LoadShaders();
	Microsoft::WRL::ComPtr<ID3D11VertexShader> LoadVertexShader(std::wstring shaderPath);
	Microsoft::WRL::ComPtr<ID3D11PixelShader> LoadPixelShader(std::wstring shaderPath);
	Microsoft::WRL::ComPtr<ID3D11InputLayout> LoadInputLayout(ID3DBlob* vertexShaderBlob);

	// Shaders and shader-related constructs
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> skyVertexShader;

	Microsoft::WRL::ComPtr<ID3D11PixelShader> basicPixelShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> debugUVsPixelShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> debugNormalsPixelShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> customPixelShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> combinedPixelShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> skyPixelShader;

	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;

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
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pavingStonesNormalsSrv;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> graffitiSrv;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> flatNormalsSrv;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rustedMetalAlbedoSrv;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rustedMetalNormalsSrv;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rustedMetalRoughnessSrv;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rustedMetalMetalnessSrv;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalPanelAlbedoSrv;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalPanelNormalsSrv;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalPanelRoughnessSrv;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalPanelMetalnessSrv;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodAlbedoSrv;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodNormalsSrv;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodRoughnessSrv;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodMetalnessSrv;

	// Materials
	std::shared_ptr<Material> pavingStonesMat;
	std::shared_ptr<Material> rustedMetalMat;
	std::shared_ptr<Material> metalPanelMat;
	std::shared_ptr<Material> woodMat;
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
	
	// Skybox
	std::shared_ptr<Skybox> skybox;
};