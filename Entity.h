#pragma once

#include <memory>
#include "Mesh.h"
#include "Transform.h"
#include "Material.h"

class Entity
{
public:
	Entity(std::shared_ptr<Mesh> pMesh, std::shared_ptr<Material> pMaterial);
	~Entity();

	void Draw(float deltaTime, float totalTime);

	// Setters
	void SetMaterial(std::shared_ptr<Material> pMaterial);

	// Getters
	Transform& GetTransform();
	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<Material> GetMaterial();
private:
	Transform transform;
	std::shared_ptr<Mesh> meshPtr;
	std::shared_ptr<Material> materialPtr;
};