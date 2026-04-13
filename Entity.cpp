#include "Entity.h"

Entity::Entity(std::shared_ptr<Mesh> pMesh, std::shared_ptr<Material> pMaterial)
{
	meshPtr = pMesh;
	materialPtr = pMaterial;
}

Entity::~Entity()
{
}

void Entity::Draw(float deltaTime)
{
	// Activate the shaders
	Graphics::Context->VSSetShader(materialPtr->GetVertexShader().Get(), 0, 0);
	Graphics::Context->PSSetShader(materialPtr->GetPixelShader().Get(), 0, 0);

	// Draw the mesh
	meshPtr->Draw(deltaTime);
}

// Setters
void Entity::SetMaterial(std::shared_ptr<Material> pMaterial)
{
	materialPtr = pMaterial;
}

// Getters
std::shared_ptr<Mesh> Entity::GetMesh()
{
	return meshPtr;
}
std::shared_ptr<Material> Entity::GetMaterial()
{
	return materialPtr;
}
Transform& Entity::GetTransform()
{
	return transform;
}
