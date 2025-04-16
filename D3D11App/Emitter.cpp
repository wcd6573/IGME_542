/*
William Duprey
4/13/25
Emitter Implementation
*/

#include "Emitter.h"
#include "Graphics.h"

// Helper macro for getting a float between min and max
#define RandomRange(min, max) ((float)rand() / RAND_MAX * (max - min) + min)

using namespace DirectX;

Emitter::Emitter(int _maxParticles, float _maxLifetime,
	int _particlesPerSecond, DirectX::XMFLOAT3 _position,
	DirectX::XMFLOAT4 _colorTint,
	std::shared_ptr<SimpleVertexShader> _vertexShader,
	std::shared_ptr<SimplePixelShader> _pixelShader,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _texture,
	Microsoft::WRL::ComPtr<ID3D11SamplerState> _sampler)
	: maxParticles(_maxParticles),
	maxLifetime(_maxLifetime),
	particlesPerSecond(_particlesPerSecond),
	colorTint(_colorTint),
	vertexShader(_vertexShader),
	pixelShader(_pixelShader),
	textureSRV(_texture),
	sampler(_sampler)
{
	// Emission rate
	secondsPerParticle = 1.0f / _particlesPerSecond;

	// Setup particle array	
	particles = new Particle[_maxParticles];
	ZeroMemory(particles, sizeof(Particle) * _maxParticles); // Necessary?

	// Set up emission fields
	indexFirstAlive = 0;
	indexFirstDead = 0;
	livingParticleCount = 0;
	timeSinceLastEmit = 0;

	// Set the emitter's position using a transform
	transform = std::make_shared<Transform>();
	transform->SetPosition(_position);

	// --- GPU Resources Setup ---
	{
		// Set up index buffer with two triangles per particle
		// - Copied straight from slides because I am too
		//   tired to be able to have figured this out
		int numIndices = maxParticles * 6;	// 3 triangles = 6 indices
		unsigned int* indices = new unsigned int[numIndices];
		int indexCount = 0;
		for (int i = 0; i < maxParticles * 4; i += 4)
		{
			indices[indexCount++] = i;
			indices[indexCount++] = i + 1;
			indices[indexCount++] = i + 2;
			indices[indexCount++] = i;
			indices[indexCount++] = i + 2;
			indices[indexCount++] = i + 3;
		}
		D3D11_SUBRESOURCE_DATA indexData = {};
		indexData.pSysMem = indices;

		// Create buffer on GPU
		D3D11_BUFFER_DESC ibDesc = {};
		ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibDesc.CPUAccessFlags = 0;
		ibDesc.Usage = D3D11_USAGE_DEFAULT;
		ibDesc.ByteWidth = sizeof(unsigned int) * maxParticles * 6;
		Graphics::Device->CreateBuffer(&ibDesc, &indexData, indexBuffer.GetAddressOf());
		delete[] indices; // Clean up CPU array

		// Make a dynamic buffer to hold all particle data on GPU
		// - Will be overwritten every frame with new lifetime data
		D3D11_BUFFER_DESC structuredDesc = {};
		structuredDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		structuredDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		structuredDesc.Usage = D3D11_USAGE_DYNAMIC;
		structuredDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		structuredDesc.StructureByteStride = sizeof(Particle);
		structuredDesc.ByteWidth = sizeof(Particle) * maxParticles;

		Graphics::Device->CreateBuffer(
			&structuredDesc,
			0,
			particleDataBuffer.GetAddressOf());

		// Create SRV that points to a structured buffer of particles
		// so we can grab this data in a vertex shader
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = maxParticles;

		Graphics::Device->CreateShaderResourceView(
			particleDataBuffer.Get(),
			&srvDesc,
			particleDataSRV.GetAddressOf());
	}
}

Emitter::~Emitter()
{
	delete[] particles;
}

void Emitter::Update(float deltaTime, float currentTime)
{
	// Only update if there is anything to update
	if (livingParticleCount > 0)
	{
		// If alive cells are not split...
		if (indexFirstAlive < indexFirstDead)
		{
			for (int i = indexFirstAlive; i < indexFirstDead; i++)
			{
				UpdateParticle(currentTime, i);
			}
		}
		// If the alive particles are split in the ring buffer
		else if (indexFirstDead < indexFirstAlive)
		{
			// Loop through first chunk
			for (int i = indexFirstAlive; i < maxParticles; i++)
			{
				UpdateParticle(currentTime, i);
			}

			// Then wrap around
			for (int i = 0; i < indexFirstDead; i++)
			{
				UpdateParticle(currentTime, i);
			}
		}
		// If alive index == dead index, loop through the whole array
		// since they might all be alive (or they might all be dead)
		else
		{
			for (int i = 0; i < maxParticles; i++)
			{
				UpdateParticle(currentTime, i);
			}
		}
	}

	// Track particle emission time and emit new particles if necessary
	timeSinceLastEmit += deltaTime;
	while (timeSinceLastEmit > secondsPerParticle)
	{
		EmitParticle(currentTime);
		timeSinceLastEmit -= secondsPerParticle;
	}
}

void Emitter::Draw(std::shared_ptr<Camera> camera, float currentTime)
{
	CopyToGPU();

	// Set up buffers
	// - No vertex buffer, vertices are constructed in the shader
	UINT stride = 0;
	UINT offset = 0;
	ID3D11Buffer* nullBuffer = 0;
	Graphics::Context->IASetVertexBuffers(0, 1, &nullBuffer, &stride, &offset);
	Graphics::Context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	// Set up shaders
	vertexShader->SetShader();
	pixelShader->SetShader();

	// Vertex shader data
	vertexShader->SetMatrix4x4("view", camera->GetView());
	vertexShader->SetMatrix4x4("projection", camera->GetProjection());
	vertexShader->SetFloat("currentTime", currentTime);
	vertexShader->CopyAllBufferData();
	
	// Set structured buffer
	vertexShader->SetShaderResourceView("ParticleData", particleDataSRV);

	// Pixel shader data
	pixelShader->SetFloat4("colorTint", colorTint);
	pixelShader->CopyAllBufferData();

	// Set other resources
	pixelShader->SetShaderResourceView("Particle", textureSRV);
	pixelShader->SetSamplerState("BasicSampler", sampler);

	// All data is set, so draw particles using DrawIndexed
	Graphics::Context->DrawIndexed(livingParticleCount * 6, 0, 0);
}

// Helper method to update a single particle's living / dead status
void Emitter::UpdateParticle(float currentTime, int index)
{
	float age = currentTime - particles[index].EmitTime;

	// If the particle is old enough to die
	if (age >= maxLifetime)
	{
		// Update indices / counts, wrap to 0 if over the ring buffer length
		indexFirstAlive++;
		indexFirstAlive %= maxParticles;
		livingParticleCount--;
	}
}

// Helper method used to emit a single particle
void Emitter::EmitParticle(float currentTime)
{
	// Don't spawn a particle if there is no room
	if (livingParticleCount == maxParticles)
	{
		return;
	}

	int i = indexFirstDead;
	
	// Update particle data as it is spawned
	particles[i].EmitTime = currentTime;
	particles[i].StartPos = transform->GetPosition();

	// Increment dead index and living count
	indexFirstDead++;
	indexFirstDead %= maxParticles;
	livingParticleCount++;
}

// Helper method to perform the CPU -> GPU memory copy
void Emitter::CopyToGPU()
{
	// Map the buffer, locking in on the GPU so we can write to it
	D3D11_MAPPED_SUBRESOURCE mapped = {};
	Graphics::Context->Map(particleDataBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

	// How are living particles arranged in the buffer?
	if (indexFirstAlive < indexFirstDead)
	{
		// Only copy from FirstAlive -> FirstDead
		memcpy(
			mapped.pData,  // Destination = start of particle buffer
			particles + indexFirstAlive, // Source = particle array, offset to first living particle
			sizeof(Particle) * livingParticleCount); // Amount = number of particles (measured in BYTES!)
	}
	else
	{
		// Copy from 0 -> First Dead
		memcpy(
			mapped.pData, // Destination = start of particle buffer
			particles,	  // Source = start of particle array
			sizeof(Particle) * indexFirstDead); // Amount = particles up to first dead (measured in BYTES!)

		// ALSO copy from FirstAlive -> End
		memcpy(
			(void*)((Particle*)mapped.pData + indexFirstDead), // Destination = particle buffer, AFTER the data we copied above
			particles + indexFirstAlive,	// Source = particle array, offset to first living particle
			sizeof(Particle) * (maxParticles - indexFirstAlive)); // Amount = number of living particles at end of array (BYTES)
	}

	//Unmap (unlock) now that we're done with it
	Graphics::Context->Unmap(particleDataBuffer.Get(), 0);
}
