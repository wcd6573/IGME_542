/*
William Duprey
2/9/25
Material Class Implementation
*/

#include "Material.h"
#include "Graphics.h"

///////////////////////////////////////////////////////////////////////////////
// ----------------------------- CONSTRUCTOR ------------------------------- //
///////////////////////////////////////////////////////////////////////////////
Material::Material(
	Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelineState, 
	DirectX::XMFLOAT3 _tint, 
	DirectX::XMFLOAT2 _scale, 
	DirectX::XMFLOAT2 _offset) 
	: pipelineState(_pipelineState),
	  colorTint(_tint),
	  uvScale(_scale),
	  uvOffset(_offset),
	  finalized(false),
	  highestSRVSlot(-1)
{
	// Initialize other data
	// - ZeroMemory is something I didn't know about, but saw in
	//   the GGP demos -- very cool. 
	finalGPUHandleForSRVs = {};
	ZeroMemory(textureSRVsBySlot, sizeof(D3D12_CPU_DESCRIPTOR_HANDLE) * 128);
}

///////////////////////////////////////////////////////////////////////////////
// ------------------------------- GETTERS --------------------------------- //
///////////////////////////////////////////////////////////////////////////////
Microsoft::WRL::ComPtr<ID3D12PipelineState> Material::GetPipelineState() { return pipelineState; }
DirectX::XMFLOAT3 Material::GetColorTint() { return colorTint; }
DirectX::XMFLOAT2 Material::GetUVScale() { return uvScale; }
DirectX::XMFLOAT2 Material::GetUVOffset() { return uvOffset; }
D3D12_GPU_DESCRIPTOR_HANDLE Material::GetFinalGPUHandleForSRVs() { return finalGPUHandleForSRVs; }

///////////////////////////////////////////////////////////////////////////////
// ------------------------------- SETTERS --------------------------------- //
///////////////////////////////////////////////////////////////////////////////
void Material::SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelineState) { pipelineState = _pipelineState; }
void Material::SetColorTint(DirectX::XMFLOAT3 _tint) { colorTint = _tint; }
void Material::SetUVScale(DirectX::XMFLOAT2 _scale) { uvScale = _scale; }
void Material::SetUVOffset(DirectX::XMFLOAT2 _offset) { uvOffset = _offset; }

// --------------------------------------------------------
// Adds a texture (with SRV descriptor) to the material
// for the given slot (gpu register). This method does
// nothing if the slot is invalid or the material has
// already been finalized.
// 
// srvDescriptorHandle - Handle to this texture's SRV
// slot - gpu register for this texture.
// 
// - Note: This comment copied from Chris Cascioli's demos
// --------------------------------------------------------
void Material::AddTexture(D3D12_CPU_DESCRIPTOR_HANDLE srv, int slot)
{
	// Don't add anything if material is already finalized 
	// or slot is out of range
	if (finalized || slot < 0 || slot > 128) {
		return;
	}

	// Add texture to the CPU descriptor handle array
	textureSRVsBySlot[slot] = srv;

	// Set highestSRV slot to the new highest slot
	highestSRVSlot = max(highestSRVSlot, slot);
}

// --------------------------------------------------------
// No way to know how many textures a material has until
// they're all added, so this method is used to trigger that.
// Individual texture descriptors are safely copied to a
// contiguous section of final CBV/SRV descriptor heap,
// effectively creating a descriptor table.
// --------------------------------------------------------
void Material::FinalizeMaterial()
{
	// Do nothing if the material has already been finalized
	if (finalized) {
		return;
	}

	// Store the GPU handle for the first SRV
	finalGPUHandleForSRVs = Graphics::CopySRVsToDescriptorHeapAndGetGPUDescriptorHandle(
			textureSRVsBySlot[0], 1);

	// Loop to copy the rest of the srvs
	// - Need to copy each one by one since each is currently
	//   in its own descriptor heap (not contiguous)
	for (int i = 1; i <= highestSRVSlot; i++)
	{
		Graphics::CopySRVsToDescriptorHeapAndGetGPUDescriptorHandle(
			textureSRVsBySlot[i], 1);
	}

	// Finalize the material to prevent this method from
	// running again, or from new textures being added
	finalized = true;
}
