#include "Entity.h"

Entity::Entity(std::shared_ptr<Mesh> pMesh)
{
	meshPtr = pMesh;
}

Entity::~Entity()
{
}

void Entity::Draw(float deltaTime, float totalTime)
{
	meshPtr->Draw(deltaTime, totalTime);
}

// Getters
std::shared_ptr<Mesh> Entity::GetMesh()
{
	return meshPtr;
}

Transform& Entity::GetTransform()
{
	return transform;
}
