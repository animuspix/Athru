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

	// Create the vertex shader from the vertex shader buffer
	result = device->CreateComputeShader(shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize(), NULL, &shader);

	// Release the vertex shader buffer + pixel shader buffer
	shaderBuffer->Release();
	shaderBuffer = nullptr;
}

ComputeShader::~ComputeShader()
{
	shader->Release();
	shader = nullptr;
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
