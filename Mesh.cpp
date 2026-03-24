#include "Mesh.h"
#include <array>

Mesh::Mesh(Vertex pVertices[], size_t pVertexCount, UINT pIndices[], size_t pIndexCount, std::string pMeshName)
{
	// Name
	meshName = pMeshName;

	// Vertex count
	vertexCount = pVertexCount;

	// Index count
	indexCount = pIndexCount;

	// Create buffers
	CreateBuffers(pVertices, pVertexCount, pIndices, pIndexCount);
}

Mesh::Mesh(const char* meshPath, std::string pMeshName)
{
	// First set name
	meshName = pMeshName;
	
	// Copied from OBJLoader.txt from IGME 540 MyCourses
	// Originally created by Chris Cascioli

	// File input object
	std::ifstream obj(meshPath);

	// Check for successful open
	if (!obj.is_open())
		throw std::invalid_argument("Error opening file: Invalid file path or file is inaccessible");

	// Variables used while reading the file
	std::vector<DirectX::XMFLOAT3> positions;	// Positions from the file
	std::vector<DirectX::XMFLOAT3> normals;		// Normals from the file
	std::vector<DirectX::XMFLOAT2> uvs;		// UVs from the file
	std::vector<Vertex> vertsFromFile;	// Verts from file (including duplicates)
	std::vector<Vertex> finalVertices;	// Final, de-duplicated verts
	std::vector<UINT> finalIndices;		// Indices for final verts
	char chars[100];			// String for line reading

	// Still have data left?
	while (obj.good())
	{
		// Get the line (100 characters should be more than enough)
		obj.getline(chars, 100);

		// Check the type of line
		if (chars[0] == 'v' && chars[1] == 'n')
		{
			// Read the 3 numbers directly into an XMFLOAT3
			DirectX::XMFLOAT3 norm{};
			sscanf_s(
				chars,
				"vn %f %f %f",
				&norm.x, &norm.y, &norm.z);

			// Add to the list of normals
			normals.push_back(norm);
		}
		else if (chars[0] == 'v' && chars[1] == 't')
		{
			// Read the 2 numbers directly into an XMFLOAT2
			DirectX::XMFLOAT2 uv{};
			sscanf_s(
				chars,
				"vt %f %f",
				&uv.x, &uv.y);

			// Add to the list of uv's
			uvs.push_back(uv);
		}
		else if (chars[0] == 'v')
		{
			// Read the 3 numbers directly into an XMFLOAT3
			DirectX::XMFLOAT3 pos{};
			sscanf_s(
				chars,
				"v %f %f %f",
				&pos.x, &pos.y, &pos.z);

			// Add to the positions
			positions.push_back(pos);
		}
		else if (chars[0] == 'f')
		{
			// Read the face indices into an array
			// NOTE: This assumes the given obj file contains
			//  vertex positions, uv coordinates AND normals.
			unsigned int i[12]{};
			int numbersRead = sscanf_s(
				chars,
				"f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d",
				&i[0], &i[1], &i[2],
				&i[3], &i[4], &i[5],
				&i[6], &i[7], &i[8],
				&i[9], &i[10], &i[11]);

			// If we only got the first number, chances are the OBJ
			// file has no UV coordinates.  This isn't great, but we
			// still want to load the model without crashing, so we
			// need to re-read a different pattern (in which we assume
			// there are no UVs denoted for any of the vertices)
			if (numbersRead == 1)
			{
				// Re-read with a different pattern
				numbersRead = sscanf_s(
					chars,
					"f %d//%d %d//%d %d//%d %d//%d",
					&i[0], &i[2],
					&i[3], &i[5],
					&i[6], &i[8],
					&i[9], &i[11]);

				// The following indices are where the UVs should 
				// have been, so give them a valid value
				i[1] = 1;
				i[4] = 1;
				i[7] = 1;
				i[10] = 1;

				// If we have no UVs, create a single UV coordinate
				// that will be used for all vertices
				if (uvs.size() == 0)
					uvs.push_back(DirectX::XMFLOAT2(0, 0));
			}

			// - Create the verts by looking up
			//    corresponding data from vectors
			// - OBJ File indices are 1-based, so
			//    they need to be adusted
			Vertex v1{};
			v1.Position = positions[max(i[0] - 1, 0)];
			v1.UV = uvs[max(i[1] - 1, 0)];
			v1.Normal = normals[max(i[2] - 1, 0)];

			Vertex v2{};
			v2.Position = positions[max(i[3] - 1, 0)];
			v2.UV = uvs[max(i[4] - 1, 0)];
			v2.Normal = normals[max(i[5] - 1, 0)];

			Vertex v3{};
			v3.Position = positions[max(i[6] - 1, 0)];
			v3.UV = uvs[max(i[7] - 1, 0)];
			v3.Normal = normals[max(i[8] - 1, 0)];

			// The model is most likely in a right-handed space,
			// especially if it came from Maya.  We probably want 
			// to convert to a left-handed space.  This means we 
			// need to:
			//  - Invert the Z position
			//  - Invert the normal's Z
			//  - Flip the winding order
			// We also need to flip the UV coordinate since Direct3D
			// defines (0,0) as the top left of the texture, and many
			// 3D modeling packages use the bottom left as (0,0)

			// Flip the UV's since they're probably "upside down"
			v1.UV.y = 1.0f - v1.UV.y;
			v2.UV.y = 1.0f - v2.UV.y;
			v3.UV.y = 1.0f - v3.UV.y;

			// Flip Z (LH vs. RH)
			v1.Position.z *= -1.0f;
			v2.Position.z *= -1.0f;
			v3.Position.z *= -1.0f;

			// Flip normal's Z
			v1.Normal.z *= -1.0f;
			v2.Normal.z *= -1.0f;
			v3.Normal.z *= -1.0f;

			// Add the verts to the vector (flipping the winding order)
			vertsFromFile.push_back(v1);
			vertsFromFile.push_back(v3);
			vertsFromFile.push_back(v2);

			// Was there a 4th face?
			// - 12 numbers read means 4 faces WITH uv's
			// - 8 numbers read means 4 faces WITHOUT uv's
			if (numbersRead == 12 || numbersRead == 8)
			{
				// Make the last vertex
				Vertex v4{};
				v4.Position = positions[max(i[9] - 1, 0)];
				v4.UV = uvs[max(i[10] - 1, 0)];
				v4.Normal = normals[max(i[11] - 1, 0)];

				// Flip the UV, Z pos and normal's Z
				v4.UV.y = 1.0f - v4.UV.y;
				v4.Position.z *= -1.0f;
				v4.Normal.z *= -1.0f;

				// Add a whole triangle (flipping the winding order)
				vertsFromFile.push_back(v1);
				vertsFromFile.push_back(v4);
				vertsFromFile.push_back(v3);
			}
		}
	}

	// We'll use a hash table (unordered_map) to determine
	// if any of the vertices are duplicates
	std::unordered_map<std::string, unsigned int> vertMap;
	for (auto& v : vertsFromFile)
	{
		// Create a "unique" representation of the vertex (its key)
		// Note: This isn't a super efficient method, but since strings
		//       inherently work with unordered_maps, this saves
		//       us from having to write our own custom hash function
		std::string vStr =
			std::to_string(v.Position.x) +
			std::to_string(v.Position.y) +
			std::to_string(v.Position.z) +
			std::to_string(v.Normal.x) +
			std::to_string(v.Normal.y) +
			std::to_string(v.Normal.z) +
			std::to_string(v.UV.x) +
			std::to_string(v.UV.y);

		// Prepare the index for this vertex and
		// search for the vertex in the hash table
		unsigned int index = -1;
		auto pair = vertMap.find(vStr);
		if (pair == vertMap.end())
		{
			// Vertex not found, so this
			// is the first time we've seen it
			index = (unsigned int)finalVertices.size();
			finalVertices.push_back(v);
			vertMap.insert({ vStr, index });
		}
		else
		{
			// Vert already exists, just
			// grab its index
			index = pair->second;
		}

		// Either way, save the index
		finalIndices.push_back(index);
	}

	// Close the file
	obj.close();

	// Save vertex and index counts
	vertexCount = finalVertices.size();
	indexCount = finalIndices.size();

	// NEXT: Create the actual buffers!
	CreateBuffers(&finalVertices[0], indexCount, &finalIndices[0], vertexCount);
}

void Mesh::CreateBuffers(Vertex pVertices[], size_t pVertexCount, UINT pIndices[], size_t pIndexCount)
{
	// Vertex buffer
	{
		// First, we need to describe the buffer we want Direct3D to make on the GPU
		//  - Note that this variable is created on the stack since we only need it once
		//  - After the buffer is created, this description variable is unnecessary
		D3D11_BUFFER_DESC vbd = {};
		vbd.Usage = D3D11_USAGE_IMMUTABLE;	// Will NEVER change
		vbd.ByteWidth = static_cast<UINT>(sizeof(Vertex) * vertexCount);       // number of vertices in the buffer
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER; // Tells Direct3D this is a vertex buffer
		vbd.CPUAccessFlags = 0;	// Note: We cannot access the data from C++ (this is good)
		vbd.MiscFlags = 0;
		vbd.StructureByteStride = 0;

		// Create the proper struct to hold the initial vertex data
		// - This is how we initially fill the buffer with data
		// - Essentially, we're specifying a pointer to the data to copy
		D3D11_SUBRESOURCE_DATA initialVertexData = {};
		initialVertexData.pSysMem = pVertices; // pSysMem = Pointer to System Memory

		// Actually create the buffer on the GPU with the initial data
		// - Once we do this, we'll NEVER CHANGE DATA IN THE BUFFER AGAIN
		Graphics::Device->CreateBuffer(&vbd, &initialVertexData, vertexBuffer.GetAddressOf());
	}

	// Index buffer
	{
		// Describe the buffer, as we did above, with two major differences
		//  - Byte Width (3 unsigned integers vs. 3 whole vertices)
		//  - Bind Flag (used as an index buffer instead of a vertex buffer) 
		D3D11_BUFFER_DESC ibd = {};
		ibd.Usage = D3D11_USAGE_IMMUTABLE;	// Will NEVER change
		ibd.ByteWidth = static_cast<UINT>(sizeof(unsigned int) * indexCount);	// number of indices in the buffer
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;	// Tells Direct3D this is an index buffer
		ibd.CPUAccessFlags = 0;	// Note: We cannot access the data from C++ (this is good)
		ibd.MiscFlags = 0;
		ibd.StructureByteStride = 0;

		// Specify the initial data for this buffer, similar to above
		D3D11_SUBRESOURCE_DATA initialIndexData = {};
		initialIndexData.pSysMem = pIndices; // pSysMem = Pointer to System Memory

		// Actually create the buffer with the initial data
		// - Once we do this, we'll NEVER CHANGE THE BUFFER AGAIN
		Graphics::Device->CreateBuffer(&ibd, &initialIndexData, indexBuffer.GetAddressOf());
	}
}

Mesh::~Mesh()
{

}

void Mesh::Draw(float deltaTime, float totalTime)
{
	// From Game.cpp
	// DRAW geometry
	// - Other Direct3D calls will also be necessary to do more complex things
	{
		// Set buffers in the input assembler (IA) stage
		//  - Do this ONCE PER OBJECT, since each object may have different geometry
		//  - For this demo, this step *could* simply be done once during Init()
		//  - However, this needs to be done between EACH DrawIndexed() call
		//     when drawing different geometry, so it's here as an example
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		Graphics::Context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
		Graphics::Context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		// Tell Direct3D to draw
		//  - Begins the rendering pipeline on the GPU
		//  - Do this ONCE PER OBJECT you intend to draw
		//  - This will use all currently set Direct3D resources (shaders, buffers, etc)
		//  - DrawIndexed() uses the currently set INDEX BUFFER to look up corresponding
		//     vertices in the currently set VERTEX BUFFER
		Graphics::Context->DrawIndexed(
			static_cast<UINT>(indexCount),     // The number of indices to use (we could draw a subset if we wanted)
			0,     // Offset to the first index we want to use
			0);    // Offset to add to each index when looking up vertices
	}
}

Microsoft::WRL::ComPtr<ID3D11Buffer> Mesh::GetVertexBuffer()
{
	return vertexBuffer;
}

Microsoft::WRL::ComPtr<ID3D11Buffer> Mesh::GetIndexBuffer()
{
	return indexBuffer;
}

size_t Mesh::GetVertexCount()
{
	return vertexCount;
}

size_t Mesh::GetIndexCount()
{
	return indexCount;
}

std::string Mesh::GetName()
{
	return meshName;
}