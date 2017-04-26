#include "ServiceCentre.h"
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

	// Setup the light-buffer description
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

Rasterizer::~Rasterizer()
{
	// Release the texture sampler state
	wrapSamplerState->Release();
	wrapSamplerState = nullptr;

	// Release the light-buffer
	lightBufferPttr->Release();
	lightBufferPttr = nullptr;

	// Flush any pipeline data associated with [this]
	//ServiceCentre::AccessGraphics()->GetD3D()->GetDeviceContext()->ClearState();
	//ServiceCentre::AccessGraphics()->GetD3D()->GetDeviceContext()->Flush();
}

void Rasterizer::Render(ID3D11DeviceContext* deviceContext,
						float dirLightIntensity, DirectX::XMFLOAT4 dirLightDirection,
						DirectX::XMFLOAT4 dirLightDiffuse, DirectX::XMFLOAT4 dirLightAmbient,
						DirectX::XMFLOAT4 dirLightPos,
						float* pointLightIntensities, DirectX::XMFLOAT4* pointLightDiffuseColors,
						DirectX::XMFLOAT4* pointLightPositions, fourByteUnsigned numPointLights,
						float* spotLightIntensities, DirectX::XMFLOAT4* spotLightDiffuseColors,
						DirectX::XMFLOAT4* spotLightPositions, DirectX::XMFLOAT4* spotLightDirections,
						fourByteUnsigned numSpotLights,
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
	// with it's original type (LightBuffer) which can be used to edit the
	// data itself
	// Very strange bug here, everything besides directional data is discarded
	LightBuffer* lightBufferData = (LightBuffer*)mappedResource.pData;

	// Write directional-light data to the light-buffer via [lightBufferData]
	lightBufferData->dirIntensity = DirectX::XMFLOAT4(dirLightIntensity, 0, 0, 0);
	lightBufferData->dirDirection = dirLightDirection;
	lightBufferData->dirDiffuse = dirLightDiffuse;
	lightBufferData->dirAmbient = dirLightAmbient;
	lightBufferData->dirPos = dirLightPos;

	// Write point-light data to the light-buffer via [lightBufferData]
	for (fourByteUnsigned i = 0; i < numPointLights; i += 1)
	{
		lightBufferData->pointIntensity[i] = DirectX::XMFLOAT4(pointLightIntensities[i], 0, 0, 0);
		lightBufferData->pointDiffuse[i] = pointLightDiffuseColors[i];
		lightBufferData->pointPos[i] = pointLightPositions[i];
	}

	// Store the point-light count in the light-buffer via [lightBufferData]
	lightBufferData->numPointLights = DirectX::XMUINT4(numPointLights, 0, 0, 0);

	// Write spot-light data to the light-buffer via [lightBufferData]
	for (fourByteUnsigned i = 0; i < numSpotLights; i += 1)
	{
		lightBufferData->spotIntensity[i] = DirectX::XMFLOAT4(spotLightIntensities[i], 0, 0, 0);
		lightBufferData->spotDiffuse[i] = spotLightDiffuseColors[i];
		lightBufferData->spotPos[i] = spotLightPositions[i];
		lightBufferData->spotDirection[i] = spotLightDirections[i];
	}

	// Store the spot-light cutoff angle in the light-buffer via [lightBufferData]
	lightBufferData->spotCutoffRadians = DirectX::XMFLOAT4(SPOT_CUTOFF_RADIANS, 0, 0, 0);

	// Store the spot light count in the light-buffer via [lightBufferData]
	lightBufferData->numSpotLights = DirectX::XMUINT4(numSpotLights, 0, 0, 0);

	// Store a copy of the world matrix for use in lighting calculations
	lightBufferData->worldMat = world;

	// Discard the mapping between the GPU-side [lightBufferPttr] and the CPU-side
	// [mappedResource]
	deviceContext->Unmap(lightBufferPttr, 0);

	// Update the pixel shader with the edited light buffer
	deviceContext->PSSetConstantBuffers(0, 1, &lightBufferPttr);

	// Render the newest boxecule on the pipeline with [this]
	Shader::RenderShader(deviceContext, numIndicesDrawing);
}
