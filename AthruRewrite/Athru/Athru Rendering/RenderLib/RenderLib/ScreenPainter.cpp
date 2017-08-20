#include "GPUServiceCentre.h"
#include "ScreenRect.h"
#include "ScreenPainter.h"

ScreenPainter::ScreenPainter(ID3D11Device* device, HWND windowHandle,
						     LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath) :
						     PipelineShader()
{
	// Long integer used for storing success/failure for different
	// DirectX operations
	HRESULT result;

	// Describe post-processing input elements
	D3D11_INPUT_ELEMENT_DESC postInputElements[2];
	postInputElements[0].SemanticName = "POSITION";
	postInputElements[0].SemanticIndex = 0;
	postInputElements[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	postInputElements[0].InputSlot = 0;
	postInputElements[0].AlignedByteOffset = 0;
	postInputElements[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	postInputElements[0].InstanceDataStepRate = 0;

	postInputElements[1].SemanticName = "TEXCOORD";
	postInputElements[1].SemanticIndex = 0;
	postInputElements[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	postInputElements[1].InputSlot = 0;
	postInputElements[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	postInputElements[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	postInputElements[1].InstanceDataStepRate = 0;

	// Build the object shader files with the given input element
	// descriptions
	PipelineShader::BuildShader(device, windowHandle,
								postInputElements, 2,
								vertexShaderFilePath, pixelShaderFilePath);

	// Setup the texture sampler state description
	// (we're using wrap sampling atm)
	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
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

ScreenPainter::~ScreenPainter()
{
	// Release the texture sampler state
	wrapSamplerState->Release();
	wrapSamplerState = nullptr;
}

void ScreenPainter::RenderShader(ID3D11DeviceContext* deviceContext)
{
	// Set the vertex input layout.
	deviceContext->IASetInputLayout(inputLayout);

	// Set the vertex/pixel shaders that will be used to render
	// the screen rect
	deviceContext->VSSetShader(vertShader, NULL, 0);
	deviceContext->PSSetShader(pixelShader, NULL, 0);

	// Render the screen rect
	deviceContext->DrawIndexed(GraphicsStuff::SCREEN_RECT_INDEX_COUNT, 0, 0);
}

void ScreenPainter::Render(ID3D11DeviceContext* deviceContext)
{
	// Pass a view of the display texture into the pixel-shading
	// registers
	AthruTexture2D displayTexture = AthruGPU::GPUServiceCentre::AccessTextureManager()->GetDisplayTexture(AVAILABLE_DISPLAY_TEXTURES::SCREEN_TEXTURE);
	deviceContext->PSSetShaderResources(0, 1, &(displayTexture.asReadOnlyShaderResource));

	// Initialise the pixel shader's texture sampler state with [wrapSamplerState]
	deviceContext->PSSetSamplers(0, 1, &wrapSamplerState);

	// Render the screen-rect with [this]
	RenderShader(deviceContext);

	// Resources can't be exposed through unordered-access-views (read/write allowed) and shader-
	// resource-views (read-only) simultaneously, so un-bind the local shader-resource-view
	// over here
	ID3D11ShaderResourceView* nullSRV = nullptr;
	deviceContext->PSSetShaderResources(0, 1, &nullSRV);
}
