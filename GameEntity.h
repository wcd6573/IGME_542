/*
William Duprey
2/9/25
GameEntity Header
*/

#pragma once
#include <memory>

#include "Mesh.h"
#include "Transform.h"
#include "Material.h"

// --------------------------------------------------------
// A class representing an entity in a game. 
// Contains a Transform, and a shared pointer to a Mesh.
// --------------------------------------------------------
class GameEntity
{
public:
	GameEntity(std::shared_ptr<Mesh> _mesh, 
		std::shared_ptr<Material> _material);

	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<Transform> GetTransform();
	std::shared_ptr<Material> GetMaterial();

	void SetMesh(std::shared_ptr<Mesh> _mesh);
	void SetMaterial(std::shared_ptr<Material> _material);

private:
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Transform> transform;
	std::shared_ptr<Material> material;
};

