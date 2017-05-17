#include "UtilityServiceCentre.h"
#include "VoxelGrid.h"
#include "Rasterizer.h"

Rasterizer::Rasterizer(ID3D11Device* device, HWND windowHandle,
					   LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath) :
					   PipelineShader()
{
	// Long integer used for storing success/failure for different
	// DirectX operations
	HRESULT result;

	// Describe object shader input elements
	D3D11_INPUT_ELEMENT_DESC voxelInputElements[2];
	voxelInputElements[0].SemanticName = "POSITION";
	voxelInputElements[0].SemanticIndex = 0;
	voxelInputElements[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	voxelInputElements[0].InputSlot = 0;
	voxelInputElements[0].AlignedByteOffset = 0;
	voxelInputElements[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	voxelInputElements[0].InstanceDataStepRate = 0;

	voxelInputElements[1].SemanticName = "TEXCOORD";
	voxelInputElements[1].SemanticIndex = 0;
	voxelInputElements[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	voxelInputElements[1].InputSlot = 1;
	voxelInputElements[1].AlignedByteOffset = 0;
	voxelInputElements[1].InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
	voxelInputElements[1].InstanceDataStepRate = 1;

	// Build the object shader files with the given input element
	// descriptions
	PipelineShader::BuildShader(device, windowHandle,
								voxelInputElements, 2,
								vertexShaderFilePath, pixelShaderFilePath);

	// Setup the matrix buffer description
	// (the matrix buffer stores the world, view, and projection matrices
	// before the start of each frame so the GPU can avoid creating/destroying
	// 512-bit slabs of matrix data in every shader call)
	D3D11_BUFFER_DESC matBufferDesc;
	matBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matBufferDesc.ByteWidth = sizeof(MatBuffer);
	matBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matBufferDesc.MiscFlags = 0;
	matBufferDesc.StructureByteStride = 0;

	// Create the matrix buffer
	result = device->CreateBuffer(&matBufferDesc, NULL, &matBufferLocal);

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

	// Release the stored world/view/projection
	// matrices
	matBufferLocal->Release();
	matBufferLocal = nullptr;
}

void Rasterizer::RenderShader(ID3D11DeviceContext* deviceContext)
{
	// Set the vertex input layout
	deviceContext->IASetInputLayout(inputLayout);

	// Ask the GPU to use the the vertex/pixel shaders associated with
	// [this] for the current render pass
	deviceContext->VSSetShader(vertShader, NULL, 0);
	deviceContext->PSSetShader(pixelShader, NULL, 0);

	// Render the voxel grid
	deviceContext->DrawIndexedInstanced(VOXEL_INDEX_COUNT, (fourByteUnsigned)GraphicsStuff::VOXEL_GRID_VOLUME, 0, 0, 0);
}

void Rasterizer::Render(ID3D11DeviceContext* deviceContext,
						DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection,
						ID3D11ShaderResourceView* sceneColorTexture)
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
	assert(SUCCEEDED(result));

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

	// Pass the scene texture onto the pipeline
	deviceContext->PSSetShaderResources(0, 1, &sceneColorTexture);

	// Set the GPU-side texture sampling state
	deviceContext->PSSetSamplers(0, 1, &wrapSamplerState);

	// Render the voxel grid with [this]
	RenderShader(deviceContext);
}
