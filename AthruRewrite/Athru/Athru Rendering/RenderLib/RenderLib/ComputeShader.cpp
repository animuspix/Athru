#include <d3dcompiler.h>
#include "UtilityServiceCentre.h"
#include "ComputeShader.h"

ComputeShader::ComputeShader(ID3D11Device* device, HWND windowHandle,
							 LPCWSTR shaderFilePath)
{
	// Value used to store success/failure for different DirectX operations
	fourByteUnsigned result;

	// Read the given shader into a buffer on the GPU
	ID3DBlob* shaderBuffer;
	D3DReadFileToBlob(shaderFilePath, &shaderBuffer);

	// Construct the shader object from the buffer populated above
	result = device->CreateComputeShader(shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize(), NULL, &shader);

	// Release the shader object
	shaderBuffer->Release();
	shaderBuffer = nullptr;
}

ComputeShader::~ComputeShader()
{
	shader->Release();
	shader = nullptr;
}

DirectX::XMFLOAT4 ComputeShader::ClipToViewableSpace(DirectX::XMVECTOR ssePosition)
{
	// Store the given SSE vector inside a shader-style scalar array (GPU's
	// can probably take SSE data much more quickly than scalar data because
	// the CPU wouldn't have to shuffle it out of the SIMD registers beforehand;
	// I'm only really using XMFLOAT4s at this point so I can easily discern
	// between shader constants and CPU-side dynamic vectors (for position)
	// or between CPU-side non-positional/directional vectors and CPU-side
	// positions (for everything else))
	DirectX::XMFLOAT4 scalarPos;
	DirectX::XMStoreFloat4(&scalarPos, ssePosition);

	// If the given vector is outside the voxel grid on any axis, clip it
	// back in

	// Clip any axes further than [VOXEL_GRID_WIDTH] units from the start
	// of the voxel grid (i.e. in front of the furthest voxel in the grid)
	float floatingVoxelGridWidth = (float)GraphicsStuff::VOXEL_GRID_WIDTH;
	scalarPos.x = min(scalarPos.x, (floatingVoxelGridWidth - 1));
	scalarPos.y = min(scalarPos.y, (floatingVoxelGridWidth - 1));
	scalarPos.z = min(scalarPos.z, (floatingVoxelGridWidth - 1));

	// Clip any axes that are less than zero units from the start
	// of the voxel grid (i.e. behind the grid)
	scalarPos.x = max(scalarPos.x, 0.0f);
	scalarPos.y = max(scalarPos.y, 0.0f);
	scalarPos.z = max(scalarPos.z, 0.0f);

	// Output XMFLOAT4's are likely to be compared with (functionally) 3D
	// voxel coordinates, so clip off the z-axis before returning the
	// adjusted vector
	scalarPos.w = 0;

	// Return the adjusted XMFLOAT4 copy of the initial vector
	return scalarPos;
}

// Push constructions for this class through Athru's custom allocator
void* ComputeShader::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<ComputeShader>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void ComputeShader::operator delete(void* target)
{
	return;
}
