#include "TexturedRasterizer.h"

TexturedRasterizer::TexturedRasterizer(ID3D11Device* device, HWND windowHandle,
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

	// Test if anything broke :)
	assert(SUCCEEDED(result));
}


TexturedRasterizer::~TexturedRasterizer()
{
	// Release the texture sampler state
	wrapSamplerState->Release();
	wrapSamplerState = nullptr;
}

void TexturedRasterizer::Render(ID3D11DeviceContext* deviceContext,
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

	// Draw the newest boxecule on the pipeline with [this]
	Shader::RenderShader(deviceContext, numIndicesDrawing);
}
