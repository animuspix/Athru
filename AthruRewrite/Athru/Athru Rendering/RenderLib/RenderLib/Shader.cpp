#include <assert.h>
#include "Typedefs.h"
#include "UtilityServiceCentre.h"
#include "Shader.h"

Shader::Shader(ID3D11Device* device, HWND windowHandle,
			   LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath)
{
	// Long integer used to store success/failure for DirectX operations
	HRESULT result;

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

	// Set up the vertex input layout description
	// This setup needs to match the SceneVertex struct in Boxecule + each shader
	D3D11_INPUT_ELEMENT_DESC polygonLayout[VERTEX_PROPERTY_COUNT];
	polygonLayout[0].SemanticName = "POSITION";
	polygonLayout[0].SemanticIndex = 0;
	polygonLayout[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[0].InputSlot = 0;
	polygonLayout[0].AlignedByteOffset = 0;
	polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[0].InstanceDataStepRate = 0;

	polygonLayout[1].SemanticName = "POSITION";
	polygonLayout[1].SemanticIndex = 1;
	polygonLayout[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[1].InputSlot = 0;
	polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[1].InstanceDataStepRate = 0;

	polygonLayout[2].SemanticName = "COLOR";
	polygonLayout[2].SemanticIndex = 0;
	polygonLayout[2].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[2].InputSlot = 0;
	polygonLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[2].InstanceDataStepRate = 0;

	polygonLayout[3].SemanticName = "COLOR";
	polygonLayout[3].SemanticIndex = 1;
	polygonLayout[3].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[3].InputSlot = 0;
	polygonLayout[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[3].InstanceDataStepRate = 0;

	polygonLayout[4].SemanticName = "NORMAL";
	polygonLayout[4].SemanticIndex = 0;
	polygonLayout[4].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[4].InputSlot = 0;
	polygonLayout[4].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[4].InstanceDataStepRate = 0;

	polygonLayout[5].SemanticName = "NORMAL";
	polygonLayout[5].SemanticIndex = 1;
	polygonLayout[5].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[5].InputSlot = 0;
	polygonLayout[5].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[5].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[5].InstanceDataStepRate = 0;

	polygonLayout[6].SemanticName = "TEXCOORD";
	polygonLayout[6].SemanticIndex = 0;
	polygonLayout[6].Format = DXGI_FORMAT_R32G32_FLOAT;
	polygonLayout[6].InputSlot = 0;
	polygonLayout[6].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[6].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[6].InstanceDataStepRate = 0;

	polygonLayout[7].SemanticName = "COLOR";
	polygonLayout[7].SemanticIndex = 2;
	polygonLayout[7].Format = DXGI_FORMAT_R32_FLOAT;
	polygonLayout[7].InputSlot = 0;
	polygonLayout[7].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[7].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[7].InstanceDataStepRate = 0;

	polygonLayout[8].SemanticName = "COLOR";
	polygonLayout[8].SemanticIndex = 3;
	polygonLayout[8].Format = DXGI_FORMAT_R32_FLOAT;
	polygonLayout[8].InputSlot = 0;
	polygonLayout[8].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[8].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[8].InstanceDataStepRate = 0;

	// Create the vertex input layout
	result = device->CreateInputLayout(polygonLayout, VERTEX_PROPERTY_COUNT, vertShaderBuffer->GetBufferPointer(),
									   vertShaderBuffer->GetBufferSize(), &inputLayout);

	// Release the vertex shader buffer + pixel shader buffer
	vertShaderBuffer->Release();
	vertShaderBuffer = nullptr;

	pixelShaderBuffer->Release();
	pixelShaderBuffer = nullptr;

	// Set up the description of the dynamic matrix cbuffer in the vertex shader.
	D3D11_BUFFER_DESC matrixBufferDesc;
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatBuffer);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;

	// Create a pointer to the local matrix buffer so we can easily move the buffered
	// matrices into the vertex shader's cbuffer
	result = device->CreateBuffer(&matrixBufferDesc, NULL, &matBufferLocal);

	// Test whether anything broke :P
	assert(SUCCEEDED(result));
}

Shader::~Shader()
{
	// Release the cbuffer
	matBufferLocal->Release();
	matBufferLocal = nullptr;

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
