#include "DirectionalLight.h"

DirectionalLight::DirectionalLight(ID3D11Device* device, HWND windowHandle,
								   LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath) :
								   Shader(device, windowHandle, vertexShaderFilePath, pixelShaderFilePath)
{
	// Long integer used for storing success/failure for different
	// DirectX operations
	HRESULT result;

	// Setup the texture sampler state description
	// (we're using wrap sampling atm)
	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	// Create the texture sampler state
	result = device->CreateSamplerState(&samplerDesc, &wrapSamplerState);
	assert(SUCCEEDED(result));

	// Setup the light buffer description
	D3D11_BUFFER_DESC lightBufferDesc;
	lightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	lightBufferDesc.ByteWidth = sizeof(LightBuffer);
	lightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	lightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	lightBufferDesc.MiscFlags = 0;
	lightBufferDesc.StructureByteStride = 0;

	// Create a pointer to the light buffer so we can easily access it from the
	// CPU before starting a draw call
	result = device->CreateBuffer(&lightBufferDesc, NULL, &lightBufferPttr);
	assert(SUCCEEDED(result));
}

DirectionalLight::~DirectionalLight()
{
	// Release the texture sampler state
	wrapSamplerState->Release();
	wrapSamplerState = nullptr;

	// Release the light buffer
	lightBufferPttr->Release();
	lightBufferPttr = nullptr;
}

void DirectionalLight::Render(ID3D11DeviceContext* deviceContext,
							  DirectX::XMFLOAT4 intensity, DirectX::XMFLOAT4 direction, DirectX::XMFLOAT4 tint, DirectX::XMFLOAT4 pos,
							  DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection,
							  ID3D11ShaderResourceView* texture, fourByteUnsigned numIndicesDrawing)
{
	// Call the base parameter setter to initialise the vertex shader's matrix cbuffer
	// with the world/view/projection matrices
	Shader::SetShaderParameters(deviceContext, world, view, projection);

	// Initialise the pixel shader's texture input with the given texture
	deviceContext->PSSetShaderResources(0, 1, &texture);

	// Initialise the pixel shader's texture sampler state with [wrapSamplerState]
	deviceContext->PSSetSamplers(0, 1, &wrapSamplerState);

	// Make the light buffer writable by mapping it's data onto a local variable,
	// then store the success/failure of the operation so it can be validated with
	// an [assert]
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT result = deviceContext->Map(lightBufferPttr, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	assert(SUCCEEDED(result));

	// Cast the raw data mapped onto [mappedResource] into a pointer formatted
	// with it's original type (LightBuffer), then access the pointer to edit
	// the data itself
	LightBuffer* lightBufferData = (LightBuffer*)mappedResource.pData;
	lightBufferData->intensity = intensity;
	lightBufferData->direction = direction;
	lightBufferData->tint = tint;
	lightBufferData->pos = pos;

	// Discard the mapping between the GPU-side [lightBuffer] and the CPU-side
	// [mappedResource]
	deviceContext->Unmap(lightBufferPttr, 0);

	// Update the pixel shader with the edited light buffer
	deviceContext->PSSetConstantBuffers(0, 1, &lightBufferPttr);

	// Render the newest boxecule on the pipeline with [this]
	Shader::RenderShader(deviceContext, numIndicesDrawing);
}
