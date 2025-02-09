/*
William Duprey
2/9/25
GameEntity Implementation
*/

#include "GameEntity.h"

// --------------------------------------------------------
// Constructor for a GameEntity. Creates a new Transform,
// and sets the given shared pointers for mesh and material.
// --------------------------------------------------------
GameEntity::GameEntity(std::shared_ptr<Mesh> _mesh,
	std::shared_ptr<Material> _material)
	: mesh(_mesh),
	  material(_material)
{
	transform = std::make_shared<Transform>();
}

///////////////////////////////////////////////////////////////////////////////
// ------------------------------- GETTERS --------------------------------- //
///////////////////////////////////////////////////////////////////////////////
std::shared_ptr<Mesh> GameEntity::GetMesh() { return mesh; }
std::shared_ptr<Transform> GameEntity::GetTransform() { return transform; }
std::shared_ptr<Material> GameEntity::GetMaterial() { return material; }

///////////////////////////////////////////////////////////////////////////////
// ------------------------------- SETTERS --------------------------------- //
///////////////////////////////////////////////////////////////////////////////
void GameEntity::SetMesh(std::shared_ptr<Mesh> _mesh) { mesh = _mesh; }
void GameEntity::SetMaterial(std::shared_ptr<Material> _material) { material = _material; }
