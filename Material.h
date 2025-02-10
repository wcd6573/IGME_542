/*
William Duprey
2/9/25
Material Class Header
*/

#pragma once
#include <DirectXMath.h>
#include <wrl/client.h>
#include <d3d12.h>

// --------------------------------------------------------
// A class representing a material. Contains methods for 
// adding textures and finalizing the material.
// --------------------------------------------------------
class Material
{
public:
	// Constructor
	Material(
		Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelineState,
		DirectX::XMFLOAT3 _tint = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
		DirectX::XMFLOAT2 _scale = DirectX::XMFLOAT2(1.0f, 1.0f),
		DirectX::XMFLOAT2 _offset = DirectX::XMFLOAT2(0.0f, 0.0f));

	// Getters
	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPipelineState();
	DirectX::XMFLOAT3 GetColorTint();
	DirectX::XMFLOAT2 GetUVScale();
	DirectX::XMFLOAT2 GetUVOffset();
	D3D12_GPU_DESCRIPTOR_HANDLE GetFinalGPUHandleForSRVs();

	// Setters
	void SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelineState);
	void SetColorTint(DirectX::XMFLOAT3 _tint);
	void SetUVScale(DirectX::XMFLOAT2 _scale);
	void SetUVOffset(DirectX::XMFLOAT2 _offset);

	// Material methods
	void AddTexture(D3D12_CPU_DESCRIPTOR_HANDLE srv, int slot);
	void FinalizeMaterial();

private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
	DirectX::XMFLOAT3 colorTint;
	DirectX::XMFLOAT2 uvScale;
	DirectX::XMFLOAT2 uvOffset;
	bool finalized;
	
	// Descriptor handle... handling
	D3D12_CPU_DESCRIPTOR_HANDLE textureSRVsBySlot[128];	// Max possible texture count
	int highestSRVSlot;	// Usually 4, but some materials may have fewer
	D3D12_GPU_DESCRIPTOR_HANDLE finalGPUHandleForSRVs;
};
