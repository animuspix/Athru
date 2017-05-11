#include "UtilityServiceCentre.h"
#include "Boxecule.h"
#include "Rasterizer.h"

Rasterizer::Rasterizer(ID3D11Device* device, HWND windowHandle,
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
}

Rasterizer::~Rasterizer()
{
	// Release the texture sampler state
	wrapSamplerState->Release();
	wrapSamplerState = nullptr;

	// Release the light-buffer
	lightBufferPttr->Release();
	lightBufferPttr = nullptr;
}

void Rasterizer::SetShaderParameters(ID3D11DeviceContext* deviceContext,
									 DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
{
	// Long integer used to store success/failure for DirectX operations
	HRESULT result;

	// Transpose the matrices to prepare them for the shader
	world = XMMatrixTranspose(world);
	view = XMMatrixTranspose(view);
	projection = XMMatrixTranspose(projection);

	// Expose the local matrix buffer for writing
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	result = deviceContext->Map(matBufferLocal, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

	// Get a pointer to the data in the local matrix buffer
	MatBuffer* dataPtr;
	dataPtr = (MatBuffer*)mappedResource.pData;

	// Copy the world/view/projection matrices into the local matrix buffer
	dataPtr->world = world;
	dataPtr->view = view;
	dataPtr->projection = projection;

	// Break the write-allowed connection to the local matrix buffer
	deviceContext->Unmap(matBufferLocal, 0);

	// Transfer the data in the local matrix buffer to the cbuffer within the
	// vertex shader
	deviceContext->VSSetConstantBuffers(0, 1, &matBufferLocal);

	// Test whether anything broke :P
	assert(SUCCEEDED(result));
}

void Rasterizer::RenderShader(ID3D11DeviceContext* deviceContext)
{
	// Set the vertex input layout.
	deviceContext->IASetInputLayout(inputLayout);

	// Set the vertex/pixel shaders that will be used to render the boxecule
	deviceContext->VSSetShader(vertShader, NULL, 0);
	deviceContext->PSSetShader(pixelShader, NULL, 0);

	// Render a boxecule
	deviceContext->DrawIndexedInstanced(BOXECULE_INDEX_COUNT, GraphicsStuff::VOXEL_GRID_VOLUME, 0, 0, 0);
}

void Rasterizer::Render(ID3D11DeviceContext* deviceContext,
						DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection,
						ID3D11ShaderResourceView* sceneColorTexture)
{
	// Initialise the vertex shader's cbuffer with the world/view/projection
	// matrices
	SetShaderParameters(deviceContext, world, view, projection);

	// Initialise the pixel shader's texture input with the given texture
	deviceContext->PSSetShaderResources(0, 1, &sceneColorTexture);

	// Initialise the pixel shader's texture sampler state with [wrapSamplerState]
	deviceContext->PSSetSamplers(0, 1, &wrapSamplerState);

	// Render the newest boxecule on the pipeline with [this]
	RenderShader(deviceContext);
}
