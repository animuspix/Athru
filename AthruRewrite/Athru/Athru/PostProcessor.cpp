#include "ServiceCentre.h"
#include "AthruRect.h"
#include "PostProcessor.h"

PostProcessor::PostProcessor(ID3D11Device* device, HWND windowHandle,
						     LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath,
							 ID3D11ShaderResourceView* postProcessShaderResource) :
						     Shader(device, windowHandle, vertexShaderFilePath, pixelShaderFilePath)
{
	// Long integer used for storing success/failure for different
	// DirectX operations
	HRESULT result;

	// Store the given shader-friendly post-processing input data
	postProcessInputResource = postProcessShaderResource;

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

PostProcessor::~PostProcessor()
{
	// Release the texture sampler state
	wrapSamplerState->Release();
	wrapSamplerState = nullptr;

	// Release the effect-status-buffer
	//effectStatusBufferPttr->Release();
	//effectStatusBufferPttr = nullptr;
}

void PostProcessor::Render(ID3D11DeviceContext* deviceContext,
	DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
{
	// Call the base parameter setter to initialise the vertex shader's matrix cbuffer
	// with the world/view/projection matrices
	Shader::SetShaderParameters(deviceContext, world, view, projection);

	// Initialise the pixel shader's texture input with the given texture
	deviceContext->PSSetShaderResources(0, 1, &postProcessInputResource);

	// Initialise the pixel shader's texture sampler state with [wrapSamplerState]
	deviceContext->PSSetSamplers(0, 1, &wrapSamplerState);

	// The object shader stores lighting data in a constant buffer that might still
	// be on the pipeline; explicitly nullify it here
	ID3D11Buffer* nullBuffer = nullptr;
	deviceContext->PSSetConstantBuffers(0, 1, &nullBuffer);

	// Render the screen-rect with [this]
	Shader::RenderShader(deviceContext, ATHRU_RECT_INDEX_COUNT);

	// Resources can't be render targets and shader resources simultaneously,
	// so un-bind the post-processing texture here
	ID3D11ShaderResourceView* nullSRV = nullptr;
	deviceContext->PSSetShaderResources(0, 1, &nullSRV);
}
