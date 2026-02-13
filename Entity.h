#pragma once

#include <memory>
#include "Mesh.h"
#include "Transform.h"

class Entity
{
public:
	Entity(std::shared_ptr<Mesh> pMesh);
	~Entity();

	void Draw(float deltaTime, float totalTime);

	// Getters
	Transform& GetTransform();
	std::shared_ptr<Mesh> GetMesh();
private:
	Transform transform;
	std::shared_ptr<Mesh> meshPtr;
};