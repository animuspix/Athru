#include <d3dcompiler.h>
#include "UtilityServiceCentre.h"
#include "ComputeShader.h"

ComputeShader::ComputeShader(const Microsoft::WRL::ComPtr<ID3D11Device>& device,
							 HWND windowHandle,
							 LPCWSTR shaderFilePath)
{
	// Declare a buffer to store the imported shader data
	Microsoft::WRL::ComPtr<ID3DBlob> shaderBuffer;

	// Import the given file into DirectX with [D3DReadFileToBlob(...)]
	HRESULT result = D3DReadFileToBlob(shaderFilePath, &shaderBuffer);
	assert(SUCCEEDED(result));

	// Construct the shader object from the buffer populated above
	result = device->CreateComputeShader(shaderBuffer->GetBufferPointer(),
										 shaderBuffer->GetBufferSize(),
										 NULL,
										 &shader);
	assert(SUCCEEDED(result));
}

ComputeShader::~ComputeShader()
{
	shader = nullptr;
}

// Push constructions for this class through Athru's custom allocator
void* ComputeShader::operator new(size_t size)
{
	StackAllocator* allocator = AthruCore::Utility::AccessMemory();
	return allocator->AlignedAlloc(size, (uByte)std::alignment_of<ComputeShader>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void ComputeShader::operator delete(void* target)
{
	return;
}
