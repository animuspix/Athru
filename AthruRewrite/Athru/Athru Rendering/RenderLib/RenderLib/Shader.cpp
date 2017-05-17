#include <assert.h>
#include "UtilityServiceCentre.h"
#include "Shader.h"

Shader::Shader()
{
	// Construction occurs in child classes
}

Shader::~Shader()
{
	// Release the vertex input layout
	inputLayout->Release();
	inputLayout = nullptr;

	// Release the pixel shader
	pixelShader->Release();
	pixelShader = nullptr;

	// Release the vertex shader
	vertShader->Release();
	vertShader = nullptr;
}

void Shader::BuildShader(ID3D11Device* device, HWND windowHandle,
						 D3D11_INPUT_ELEMENT_DESC* inputElementInfo, fourByteUnsigned numInputElements,
			   			 LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath)
{
	// Value used to store success/failure for different DirectX operations
	fourByteUnsigned result;

	// Read the vertex shader into a buffer on the GPU
	ID3DBlob* vertShaderBuffer;
	D3DReadFileToBlob(vertexShaderFilePath, &vertShaderBuffer);

	// Read the pixel shader into a buffer on the GPU
	ID3DBlob* pixelShaderBuffer;
	D3DReadFileToBlob(pixelShaderFilePath, &pixelShaderBuffer);

	// Create the vertex shader from the vertex shader buffer
	result = device->CreateVertexShader(vertShaderBuffer->GetBufferPointer(), vertShaderBuffer->GetBufferSize(), NULL, &vertShader);

	// Create the pixel shader from the pixel shader buffer
	result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pixelShader);

	// Create the vertex input layout
	result = device->CreateInputLayout(inputElementInfo, numInputElements, vertShaderBuffer->GetBufferPointer(),
									   vertShaderBuffer->GetBufferSize(), &inputLayout);

	// Release the vertex shader buffer + pixel shader buffer
	vertShaderBuffer->Release();
	vertShaderBuffer = nullptr;

	pixelShaderBuffer->Release();
	pixelShaderBuffer = nullptr;
}

// Push constructions for this class through Athru's custom allocator
void* Shader::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<Shader>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void Shader::operator delete(void* target)
{
	return;
}
