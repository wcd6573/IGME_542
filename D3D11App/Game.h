/*
William Duprey
5/2/25
Game Header
 - Starter code provided by Prof. Chris Cascioli
*/

#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include <memory>

#include "Mesh.h"
#include "GameEntity.h"
#include "Camera.h"
#include "Material.h"
#include "SimpleShader.h"
#include "Lights.h"
#include "Sky.h"

class Game
{
public:
	// Basic OOP setup
	Game() = default;
	~Game();
	Game(const Game&) = delete; // Remove copy constructor
	Game& operator=(const Game&) = delete; // Remove copy-assignment operator

	// Primary functions
	void Initialize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);
	void OnResize();

private:

	// Initialization helper methods - feel free to customize, combine, remove, etc.
	void LoadAssetsAndCreateEntities();

	// Helper for creating a solid color texture & SRV
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateSolidColorTextureSRV(int width, int height, DirectX::XMFLOAT4 color);

	// General helpers for setup and drawing
	void RandomizeEntities();
	void GenerateLights();
	void DrawLightSources();
	void SetupMRT();
	void CreateRandom4x4TextureAndOffsetArray();

	// Camera for the 3D scene
	std::shared_ptr<FPSCamera> camera;

	// The sky box
	std::shared_ptr<Sky> sky;

	// Scene data
	std::vector<std::shared_ptr<Mesh>> meshes;
	std::vector<std::shared_ptr<Material>> materials;
	std::vector<std::shared_ptr<GameEntity>> entitiesRandom;
	std::vector<std::shared_ptr<GameEntity>> entitiesLineup;
	std::vector<std::shared_ptr<GameEntity>> entitiesGradient;
	std::vector<std::shared_ptr<GameEntity>>* currentScene;
	std::vector<Light> lights;
	
	// Overall lighting options
	DemoLightingOptions lightOptions;
	std::shared_ptr<Mesh> pointLightMesh;

	// Shaders (for shader swapping between pbr and non-pbr)
	std::shared_ptr<SimplePixelShader> pixelShader;
	std::shared_ptr<SimplePixelShader> pixelShaderPBR;

	// Shaders for solid color spheres
	std::shared_ptr<SimplePixelShader> solidColorPS;
	std::shared_ptr<SimpleVertexShader> vertexShader;

	// --- SSAO Fields ---
	// Shaders
	std::shared_ptr<SimplePixelShader> occlusionPS;
	std::shared_ptr<SimplePixelShader> occlusionBlurPS;
	std::shared_ptr<SimplePixelShader> occlusionCombinePS;
	std::shared_ptr<SimpleVertexShader> fullscreenVS;
	
	// Samplers
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> clampSampler;

	// SSAO data
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> randomTextureSRV;
	int ssaoSamples;	// Must be between 1 and 64
	float ssaoRadius;
	DirectX::XMFLOAT4* ssaoOffsets;

	// Textures, RTVs, and SRVs
	// Scene colors
	Microsoft::WRL::ComPtr<ID3D11Texture2D> sceneColorsTexture;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> sceneColorsRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sceneColorsSRV;

	// Ambient colors
	Microsoft::WRL::ComPtr<ID3D11Texture2D> ambientTexture;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> ambientRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ambientSRV;

	// Scene depths
	Microsoft::WRL::ComPtr<ID3D11Texture2D> sceneDepthTexture;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> sceneDepthRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sceneDepthSRV;

	// Scene normals
	Microsoft::WRL::ComPtr<ID3D11Texture2D> sceneNormalTexture;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> sceneNormalRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sceneNormalSRV;

	// SSAO Results
	Microsoft::WRL::ComPtr<ID3D11Texture2D> ssaoResultTexture;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> ssaoResultRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ssaoResultSRV;

	// Blurred SSAO
	Microsoft::WRL::ComPtr<ID3D11Texture2D> blurSSAOTexture;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> blurSSAORTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> blurSSAOSRV;
};

