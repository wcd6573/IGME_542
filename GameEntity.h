/*
William Duprey
2/1/25
GameEntity Header
*/

#pragma once
#include <memory>

#include "Mesh.h"
#include "Transform.h"

// --------------------------------------------------------
// A class representing an entity in a game. 
// Contains a Transform, and a shared pointer to a Mesh.
// --------------------------------------------------------
class GameEntity
{
public:
	GameEntity(std::shared_ptr<Mesh> _mesh);

	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<Transform> GetTransform();
	
	void SetMesh(std::shared_ptr<Mesh> _mesh);

private:
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Transform> transform;
};

