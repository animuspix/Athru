#include <assert.h>
#include <d3dcompiler.h>
#include "GPUServiceCentre.h"
#include "UtilityServiceCentre.h"
#include "PipelineShader.h"

PipelineShader::PipelineShader()
{
	// Construction occurs in child classes
}

PipelineShader::~PipelineShader() {}

void PipelineShader::BuildShader(const Microsoft::WRL::ComPtr<ID3D11Device>& device,
								 HWND windowHandle,
								 D3D11_INPUT_ELEMENT_DESC* inputElementInfo, fourByteUnsigned numInputElements,
			   					 LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath)
{
	// Value used to store success/failure for different DirectX operations
	fourByteUnsigned result;

	// Read the vertex shader into a buffer on the GPU
	Microsoft::WRL::ComPtr<ID3DBlob> vertShaderBuffer;
	D3DReadFileToBlob(vertexShaderFilePath, &vertShaderBuffer);

	// Read the pixel shader into a buffer on the GPU
	Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBuffer;
	D3DReadFileToBlob(pixelShaderFilePath, &pixelShaderBuffer);

	// Create the vertex shader from the vertex shader buffer
	result = device->CreateVertexShader(vertShaderBuffer->GetBufferPointer(), vertShaderBuffer->GetBufferSize(), NULL, &vertShader);

	// Create the pixel shader from the pixel shader buffer
	result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pixelShader);

	// Create the vertex input layout
	result = device->CreateInputLayout(inputElementInfo, numInputElements, vertShaderBuffer->GetBufferPointer(),
									   vertShaderBuffer->GetBufferSize(), &inputLayout);
}

// Push constructions for this class through Athru's custom allocator
void* PipelineShader::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<PipelineShader>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void PipelineShader::operator delete(void* target)
{
	return;
}
