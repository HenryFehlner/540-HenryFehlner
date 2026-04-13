#pragma once

#include "Graphics.h"
#include "Vertex.h"
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <unordered_map>

class Mesh
{
public:
	Mesh(Vertex pVertices[], size_t pVertexCount, UINT pIndices[], size_t pIndexCount, std::string pMeshName);
	Mesh(const char* meshPath, std::string pMeshName = "Mesh");
	~Mesh();

	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();

	size_t GetVertexCount();
	size_t GetIndexCount();
	std::string GetName();

	void Draw(float deltaTime);
private:
	// Buffer creation methods
	void CreateBuffers(Vertex pVertices[], size_t pVertexCount, UINT pIndices[], size_t pIndexCount);
	void CalculateTangents(Vertex* verts, int numVerts, unsigned int* indices, int numIndices);

	// Buffers for geometry data
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

	// Vertex and index counts
	size_t vertexCount;
	size_t indexCount;

	// More mesh info
	std::string meshName;
};