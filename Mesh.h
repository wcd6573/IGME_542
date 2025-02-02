// William Duprey
// 10/16/24
// Mesh Class Header

#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include "Graphics.h"
#include "Vertex.h"


// --------------------------------------------------------
// A class that stores data on a 3D mesh. Rather than be 
// a "dumb" container, it can draw itself.
// --------------------------------------------------------
class Mesh
{
public:
	// Underscores used for potentially ambiguous param names
	Mesh(Vertex* vertices, size_t _vertexCount,
		UINT* indices, size_t _indexCount,
		const char* _name);
	Mesh(const char* _name, const char* objFile);
	~Mesh();

	// No copy constructor and copy assignment operator
	// (scary, and I don't want to deal with them)

	// Getters for the private fields
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
	UINT GetIndexCount();
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();
	UINT GetVertexCount();
	const char* GetName();

	// Sets buffers and draws the mesh to the screen
	void SetBuffersAndDraw();

private:
	// Helper method provided by Chris Cascioli
	void CalculateTangents(Vertex* verts, int numVerts,
		unsigned int* indices, int numIndices);

	// Helper method for creating vertex and index buffers
	void CreateBuffers(Vertex* vertices, size_t _vertexCount,
		UINT* indices, size_t _indexCount);

	// Vertices of the triangles making up the mesh
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	UINT vertexCount;

	// Indices of the vertices of the triangles making up the mesh
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	UINT indexCount;

	// Name of the mesh for ImGui to display
	const char* name;
};

