/*
William Duprey
2/1/25
GameEntity Implementation
*/

#include "GameEntity.h"

// --------------------------------------------------------
// Constructor for a GameEntity. Creates a new Transform,
// and sets the given shared pointer to a Mesh.
// --------------------------------------------------------
GameEntity::GameEntity(std::shared_ptr<Mesh> _mesh)
	: mesh(_mesh)
{
	transform = std::make_shared<Transform>();
}

///////////////////////////////////////////////////////////////////////////////
// ------------------------------- GETTERS --------------------------------- //
///////////////////////////////////////////////////////////////////////////////
std::shared_ptr<Mesh> GameEntity::GetMesh() { return mesh; }
std::shared_ptr<Transform> GameEntity::GetTransform() { return transform; }

///////////////////////////////////////////////////////////////////////////////
// ------------------------------- SETTERS --------------------------------- //
///////////////////////////////////////////////////////////////////////////////
void GameEntity::SetMesh(std::shared_ptr<Mesh> _mesh) { mesh = _mesh; }
