#include <d3dcompiler.h>
#include "UtilityServiceCentre.h"
#include "ComputeShader.h"

ComputeShader::ComputeShader(ID3D11Device* device, HWND windowHandle,
	LPCWSTR shaderFilePath)
{
	// Value used to store success/failure for different DirectX operations
	fourByteUnsigned result;

	// Declare a buffer to store the imported shader data
	ID3DBlob* shaderBuffer;

	// Import the given file into DirectX with [D3DReadFileToBlob(...)]
	result = D3DReadFileToBlob(shaderFilePath, &shaderBuffer);
	assert(SUCCEEDED(result));

	// Construct the shader object from the buffer populated above
	result = device->CreateComputeShader(shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize(), NULL, &shader);
	assert(SUCCEEDED(result));

	// Release the shader object
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
