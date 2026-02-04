#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include "Mesh.h"

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
	void LoadShaders();
	//void CreateGeometry();

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	//// Buffers to hold actual geometry data
	//Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	//Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

	// Shaders and shader-related constructs
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;

	// ImGui update helper
	void ImGuiNewFrameUpdate(float deltaTime);
	void ImGuiBuildUI();

	// Mesh container
	std::vector<std::shared_ptr<Mesh>> meshVec;

	// Test meshes (commented out to use them with the vector, I'm leaving 
	// them here because I don't know how we will expand later)
	//std::shared_ptr<Mesh> triangleMesh;
	//std::shared_ptr<Mesh> quadMesh;
	//std::shared_ptr<Mesh> weirdMesh;

	// Constant buffer
	Microsoft::WRL::ComPtr<ID3D11Buffer> constBuffer;
};

