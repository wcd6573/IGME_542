/*
William Duprey
2/2/25
Game Implementation
- Starter code provided by Prof. Chris Cascioli.
*/

#include "Game.h"
#include "Graphics.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "Window.h"
#include "BufferStructs.h"
#include "RayTracing.h"

#include <DirectXMath.h>
#include <WICTextureLoader.h>

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

// Helper macro for getting a float between min and max
// - Copied directly from Chris Cascioli's demo code
#define RandomRange(min, max) (float)rand() / RAND_MAX * (max - min) + min

// --------------------------------------------------------
// Called once per program, after the window and graphics API
// are initialized but before the game loop begins
// --------------------------------------------------------
void Game::Initialize()
{
	// Initialize raytracing before loading any meshes
	RayTracing::Initialize(
		Window::Width(),
		Window::Height(),
		FixPath(L"RayTracing.cso"));

	CreateEntities();
	CreateLights();

	// Create camera, and aim it slightly downwards
	camera = std::make_shared<Camera>(
		XMFLOAT3(-0.5f, 6.25f, -15.5f),
		Window::AspectRatio());
	camera->GetTransform()->SetRotation(0.366f, 0.0f, 0.0f);

	// Finalize any initialization and wait for the GPU
	// before proceeding to the game loop
	Graphics::CloseAndExecuteCommandList();
	Graphics::WaitForGPU();
	Graphics::ResetAllocatorAndCommandList();
}


// --------------------------------------------------------
// Clean up memory or objects created by this class
// 
// Note: Using smart pointers means there probably won't
//       be much to manually clean up here!
// --------------------------------------------------------
Game::~Game()
{
	// Wait for the GPU before we shut down
	Graphics::WaitForGPU();
}


void Game::CreateLights()
{
	lights.clear();
	srand((unsigned int)time(0));
	lightCount = 16;

	// --- Create Lights ---
	Light directional1 = {};
	directional1.Type = LIGHT_TYPE_DIRECTIONAL;
	directional1.Direction = XMFLOAT3(1.0f, 0.0f, 0.0f);
	directional1.Color = XMFLOAT3(1.0f, 0.0f, 0.0f);
	directional1.Intensity = 1.0f;

	// Primary, shadow-casting light
	Light directional2 = {};
	directional2.Type = LIGHT_TYPE_DIRECTIONAL;
	directional2.Direction = XMFLOAT3(0.0f, -1.0f, 0.5f);
	directional2.Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	directional2.Intensity = 1.0f;

	Light directional3 = {};
	directional3.Type = LIGHT_TYPE_DIRECTIONAL;
	directional3.Direction = XMFLOAT3(0.5f, -1.0f, -1.0f);
	directional3.Color = XMFLOAT3(0.0f, 0.0f, 1.0f);
	directional3.Intensity = 1.0f;

	Light point1 = {};
	point1.Type = LIGHT_TYPE_POINT;
	point1.Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	point1.Intensity = 1.0f;
	point1.Position = XMFLOAT3(-1.5f, 0, 0);
	point1.Range = 10.0f;

	Light point2 = {};
	point2.Type = LIGHT_TYPE_POINT;
	point2.Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	point2.Intensity = 0.5f;
	point2.Position = XMFLOAT3(1.5f, 0, 0);
	point2.Range = 10.0f;

	lights.push_back(directional1);
	lights.push_back(directional2);
	lights.push_back(directional3);
	lights.push_back(point1);
	lights.push_back(point2);

	// Normalize directions for everything other than point lights
	for (int i = 0; i < lights.size(); i++)
	{
		if (lights[i].Type != LIGHT_TYPE_POINT) {
			XMStoreFloat3(
				&lights[i].Direction,
				XMVector3Normalize(XMLoadFloat3(&lights[i].Direction))
			);
		}
	}

	// Create a bunch of random point lights
	// - Copied directly from Chris Cascioli's demo code
	while (lights.size() < MAX_LIGHTS)
	{
		Light point = {};
		point.Type = LIGHT_TYPE_POINT;
		point.Position = XMFLOAT3(RandomRange(-15.0f, 15.0f), RandomRange(-2.0f, 5.0f), RandomRange(-15.0f, 15.0f));
		point.Color = XMFLOAT3(RandomRange(0, 1), RandomRange(0, 1), RandomRange(0, 1));
		point.Range = RandomRange(5.0f, 10.0f);
		point.Intensity = RandomRange(0.1f, 3.0f);
	
		lights.push_back(point);
	}
}


// --------------------------------------------------------
// Creates the geometry we're going to draw
// --------------------------------------------------------
void Game::CreateEntities()
{
	// --- Load models --- 
	std::shared_ptr<Mesh> cube = std::make_shared<Mesh>("Cube",
		FixPath("../../Assets/Models/cube.obj").c_str());
	std::shared_ptr<Mesh> cylinder = std::make_shared<Mesh>("Cylinder",
		FixPath("../../Assets/Models/cylinder.obj").c_str());
	std::shared_ptr<Mesh> helix = std::make_shared<Mesh>("Helix",
		FixPath("../../Assets/Models/helix.obj").c_str());
	std::shared_ptr<Mesh> sphere = std::make_shared<Mesh>("Sphere",
		FixPath("../../Assets/Models/sphere.obj").c_str());
	std::shared_ptr<Mesh> torus = std::make_shared<Mesh>("Torus",
		FixPath("../../Assets/Models/torus.obj").c_str());
	std::shared_ptr<Mesh> quad = std::make_shared<Mesh>("Quad",
		FixPath("../../Assets/Models/quad.obj").c_str());
	std::shared_ptr<Mesh> quadDouble = std::make_shared<Mesh>("Quad Double Sided",
		FixPath("../../Assets/Models/quad_double_sided.obj").c_str());

	meshes.push_back(cube);
	meshes.push_back(cylinder);
	meshes.push_back(helix);
	meshes.push_back(sphere);
	meshes.push_back(torus);
	meshes.push_back(quad);
	meshes.push_back(quadDouble);

	// Create a BLAS for a single  mesh, then the TLAS for our "scene"
	RayTracing::CreateBottomLevelAccelerationStructureForMesh(sphere.get());
	RayTracing::CreateTopLevelAccelerationStructureForScene();
}


// --------------------------------------------------------
// Handle resizing to match the new window size
//  - Also updates the camera projection matrix
// --------------------------------------------------------
void Game::OnResize()
{
	// Only calculate projection matrix if there's actually
	// a camera to use (OnResize can be called before it is
	// initialized, which leads to some problems).
	if (camera)
	{
		camera->UpdateProjectionMatrix(Window::AspectRatio());
	}

	// Resize raytracing output texture
	RayTracing::ResizeOutputUAV(Window::Width(), Window::Height());
}


// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Example input checking: Quit if the escape key is pressed
	if (Input::KeyDown(VK_ESCAPE))
		Window::Quit();

	// Loop through entities to make them rotate
	for (auto e : entities)
	{
		e->GetTransform()->Rotate(0.0f, deltaTime, 0.0f);
	}

	camera->Update(deltaTime);
}


// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Grab the current back buffer for this frame
	Microsoft::WRL::ComPtr<ID3D12Resource> currentBackBuffer =
		Graphics::BackBuffers[Graphics::SwapChainIndex()];

	// Perform ray trace (which also copies the results to the back buffer)
	RayTracing::Raytrace(camera, currentBackBuffer);

	// Present
	{
		// Must occur BEFORE present
		Graphics::CloseAndExecuteCommandList();

		// Present the current back buffer and move to the next one
		bool vsync = Graphics::VsyncState();
		Graphics::SwapChain->Present(
			vsync ? 1 : 0,
			vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING);
		Graphics::AdvanceSwapChainIndex();

		// Wait for the GPU to be done and then reset the command list & allocator
		Graphics::WaitForGPU();
		Graphics::ResetAllocatorAndCommandList();
	}
}



