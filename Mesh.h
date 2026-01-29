#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include "Graphics.h"
#include "Vertex.h"

class Mesh
{
public:
	Mesh(Vertex pVertices[], size_t pVertexCount, unsigned int pIndices[], size_t pIndexCount, std::string pMeshName);
	~Mesh();

	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();

	size_t GetVertexCount();
	size_t GetIndexCount();
	std::string GetName();

	void Draw(float deltaTime, float totalTime);
private:
	// Buffers for geometry data
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

	// Vertex and index counts
	size_t vertexCount;
	size_t indexCount;

	// More mesh info
	std::string meshName;
};