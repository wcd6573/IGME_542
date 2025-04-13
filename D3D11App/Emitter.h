/*
William Duprey
4/13/25
Emitter Header
*/

#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <memory>
#include "SimpleShader.h"
#include "Camera.h"

// Struct representing a single particle in the system
struct Particle {
	float EmitTime;
	DirectX::XMFLOAT3 StartPos;
};

// Class that contains particles, and emits / updates them
class Emitter
{
public:
	Emitter(unsigned int _maxParticles, float _maxLifetime,
		unsigned int particlesPerSecond, float timePerEmission,
		std::shared_ptr<SimpleVertexShader> _vertexShader,
		std::shared_ptr<SimplePixelShader> _pixelShader);
	~Emitter();

	void Update(float deltaTime, float currentTime);
	void Draw(std::shared_ptr<Camera> camera, float currentTime);

private:
	// --- Particle array fields ---
	unsigned int maxParticles;
	Particle* particles;
	unsigned int indexFirstAlive;
	unsigned int indexFirstDead;
	unsigned int totalLivingCount;

	// --- Emission properties ---
	float maxLifetime;
	unsigned int particlesPerSecond;
	float timePerEmission;
	float emitTimer;

	// --- GPU Resources ---
	Microsoft::WRL::ComPtr<ID3D11Buffer> particleDataBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleDataSRV;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureSRV;
	std::shared_ptr<SimpleVertexShader> vertexShader;
	std::shared_ptr<SimplePixelShader> pixelShader;
};

