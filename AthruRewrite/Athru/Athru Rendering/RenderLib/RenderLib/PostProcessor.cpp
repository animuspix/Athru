#include "UtilityServiceCentre.h"
#include "ScreenRect.h"
#include "PostProcessor.h"

PostProcessor::PostProcessor(ID3D11Device* device, HWND windowHandle,
						     LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath,
							 ID3D11ShaderResourceView* postProcessShaderResource) :
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

	// Store the given shader-friendly post-processing input texture
	postProcessInputResource = postProcessShaderResource;

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

	// Setup the effect-status buffer description
	D3D11_BUFFER_DESC effectStatusBufferDesc;
	effectStatusBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	effectStatusBufferDesc.ByteWidth = sizeof(EffectStatusBuffer);
	effectStatusBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	effectStatusBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	effectStatusBufferDesc.MiscFlags = 0;
	effectStatusBufferDesc.StructureByteStride = 0;

	// Create a pointer to the effect-status buffer so we can easily access it from the
	// CPU before starting a draw call
	result = device->CreateBuffer(&effectStatusBufferDesc, NULL, &effectStatusBufferPttr);
	assert(SUCCEEDED(result));
}

PostProcessor::~PostProcessor()
{
	// Release the texture sampler state
	wrapSamplerState->Release();
	wrapSamplerState = nullptr;

	// Release the effect-status-buffer
	effectStatusBufferPttr->Release();
	effectStatusBufferPttr = nullptr;
}

void PostProcessor::RenderShader(ID3D11DeviceContext* deviceContext)
{
	// Set the vertex input layout.
	deviceContext->IASetInputLayout(inputLayout);

	// Set the vertex/pixel shaders that will be used to render the boxecule
	deviceContext->VSSetShader(vertShader, NULL, 0);
	deviceContext->PSSetShader(pixelShader, NULL, 0);

	// Render a boxecule
	deviceContext->DrawIndexed(SCREEN_RECT_INDEX_COUNT, 0, 0);
}

void PostProcessor::Render(ID3D11DeviceContext* deviceContext,
						   bool useBlur, bool useDrugs, bool brightenScene)
{
	// Initialise the pixel shader's texture input with the given texture
	deviceContext->PSSetShaderResources(0, 1, &postProcessInputResource);

	// Initialise the pixel shader's texture sampler state with [wrapSamplerState]
	deviceContext->PSSetSamplers(0, 1, &wrapSamplerState);

	// Make the light buffer writable by mapping it's data onto a local variable,
	// then store the success/failure of the operation so it can be validated with
	// an [assert]
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT result = deviceContext->Map(effectStatusBufferPttr, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	assert(SUCCEEDED(result));

	// Cast the raw data mapped onto [mappedResource] into a pointer formatted
	// with it's original type (LightBuffer) which can be used to edit the
	// data itself
	// Very strange bug here, everything besides directional data is discarded
	EffectStatusBuffer* effectStatusBufferData = (EffectStatusBuffer*)mappedResource.pData;

	// Write directional-light data to the light-buffer via [lightBufferData]
	effectStatusBufferData->blurActive = DirectX::XMINT4(useBlur, useBlur, useBlur, useBlur);
	effectStatusBufferData->drugsActive = DirectX::XMINT4(useDrugs, useDrugs, useDrugs, useDrugs);
	effectStatusBufferData->brightenActive = DirectX::XMINT4(brightenScene, brightenScene, brightenScene, brightenScene);

	// Discard the mapping between the GPU-side [lightBufferPttr] and the CPU-side
	// [mappedResource]
	deviceContext->Unmap(effectStatusBufferPttr, 0);

	// Update the pixel shader with the edited effect-status buffer
	deviceContext->PSSetConstantBuffers(0, 1, &effectStatusBufferPttr);

	// Render the screen-rect with [this]
	RenderShader(deviceContext);

	// Resources can't be render targets and shader resources simultaneously,
	// so un-bind the post-processing texture here
	ID3D11ShaderResourceView* nullSRV = nullptr;
	deviceContext->PSSetShaderResources(0, 1, &nullSRV);
}
