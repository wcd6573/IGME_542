/*
William Duprey
4/16/25
Emitter Header
*/

#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <memory>
#include "SimpleShader.h"
#include "Camera.h"
#include "Transform.h"

// Struct representing a single particle in the system
struct Particle {
	float EmitTime;
	DirectX::XMFLOAT3 StartPos;
};

// Class that contains particles, and emits / updates them
class Emitter
{
public:
	Emitter(
		int _maxParticles, 
		float _maxLifetime,
		int _particlesPerSecond, 
		DirectX::XMFLOAT3 _position,
		float _startSize,
		float _endSize,
		DirectX::XMFLOAT4 _startColor, 
		DirectX::XMFLOAT4 _endColor,
		std::shared_ptr<SimpleVertexShader> _vertexShader,
		std::shared_ptr<SimplePixelShader> _pixelShader,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _texture,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> _sampler);
	~Emitter();

	void Update(float deltaTime, float currentTime);
	void Draw(std::shared_ptr<Camera> camera, float currentTime);

private:
	// --- Particle array fields ---
	int maxParticles;
	Particle* particles;
	int indexFirstAlive;
	int indexFirstDead;
	int livingParticleCount;

	// --- Emission properties ---
	float maxLifetime;
	int particlesPerSecond;
	float secondsPerParticle;
	float timeSinceLastEmit;
	std::shared_ptr<Transform> transform;
	
	// --- Particle properties ---
	float startSize;
	float endSize;
	DirectX::XMFLOAT4 startColor;
	DirectX::XMFLOAT4 endColor;

	// --- GPU Resources ---
	Microsoft::WRL::ComPtr<ID3D11Buffer> particleDataBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleDataSRV;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureSRV;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
	std::shared_ptr<SimpleVertexShader> vertexShader;
	std::shared_ptr<SimplePixelShader> pixelShader;

	// --- Helper methods ---
	void UpdateParticle(float currentTime, int index);
	void EmitParticle(float currentTime);
	void CopyToGPU();
};