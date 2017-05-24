#include "UtilityServiceCentre.h"
#include "SceneIlluminator.h"

SceneIlluminator::SceneIlluminator(ID3D11Device* device, HWND windowHandle,
								   LPCWSTR shaderFilePath,
								   ID3D11UnorderedAccessView* writableColorShaderResource,
								   ID3D11ShaderResourceView* sceneNormalShaderResource,
								   ID3D11ShaderResourceView* scenePBRShaderResource,
								   ID3D11ShaderResourceView* sceneEmissivityShaderResource) :
								   sceneColorTexture(writableColorShaderResource),
								   sceneNormalTexture(sceneNormalShaderResource),
								   scenePBRTexture(scenePBRShaderResource),
								   sceneEmissivityTexture(sceneEmissivityShaderResource),
								   ComputeShader(device, windowHandle,
								   			     shaderFilePath)
{
	// Long integer used for storing success/failure for different
	// DirectX operations
	HRESULT result;

	// Setup the camera-data buffer description
	D3D11_BUFFER_DESC cameraDataBufferDesc;
	cameraDataBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cameraDataBufferDesc.ByteWidth = sizeof(CameraDataBuffer);
	cameraDataBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cameraDataBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cameraDataBufferDesc.MiscFlags = 0;
	cameraDataBufferDesc.StructureByteStride = 0;

	// Create a pointer to the camera-data buffer so we can easily access it from the
	// CPU before starting a draw call
	result = device->CreateBuffer(&cameraDataBufferDesc, NULL, &cameraDataBufferPttr);
	assert(SUCCEEDED(result));
}

SceneIlluminator::~SceneIlluminator()
{
	// Release the camera-data buffer
	cameraDataBufferPttr->Release();
	cameraDataBufferPttr = nullptr;
}

void SceneIlluminator::Dispatch(ID3D11DeviceContext* context,
								DirectX::XMVECTOR cameraViewVector)
{
	// Long integer used for storing success/failure for different
	// DirectX operations
	HRESULT result;

	// Define the shader object associated with [this] as the current compute
	// shader on the GPU
	context->CSSetShader(shader, 0, 0);

	// Pass the writable shader resources associated with [this] onto the GPU
	context->CSSetUnorderedAccessViews(0, 1, &sceneColorTexture, 0);

	// Pass the read-only shader resources associated with [this] onto the GPU
	context->CSSetShaderResources(0, 1, &sceneNormalTexture);
	context->CSSetShaderResources(1, 1, &scenePBRTexture);
	context->CSSetShaderResources(2, 1, &sceneEmissivityTexture);

	// Define the given per-dispatch shader constants on the GPU

	// Make the camera-data buffer writable by mapping it's data onto a local variable
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	result = context->Map(cameraDataBufferPttr, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	assert(SUCCEEDED(result));

	// Cast the raw data mapped onto [mappedResource] into a pointer formatted
	// with it's original type (CameraDataBuffer) which can be used to edit the
	// data itself
	CameraDataBuffer* cameraDataBufferMemory = (CameraDataBuffer*)mappedResource.pData;

	// Write system data to the camera-data buffer via [cameraDataBufferMemory]
	cameraDataBufferMemory->cameraViewVec = MathsStuff::sseToScalarVector(cameraViewVector);

	// Discard the mapping between the GPU-side [cameraDataBufferPttr] and the CPU-side
	// [mappedResource]
	context->Unmap(cameraDataBufferPttr, 0);

	// Update the shader object with the edited camera-data buffer
	context->CSSetConstantBuffers(0, 1, &cameraDataBufferPttr);

	// Begin processing the shader
	context->Dispatch(GraphicsStuff::VOXEL_GRID_WIDTH, GraphicsStuff::VOXEL_GRID_WIDTH, GraphicsStuff::VOXEL_GRID_WIDTH);

	// Resources can't be bound in more than one place at once, so unbind
	// them here
	ID3D11UnorderedAccessView* nullUAV = nullptr;
	context->CSSetUnorderedAccessViews(0, 1, &nullUAV, 0);

	ID3D11ShaderResourceView* nullSRV = nullptr;
	context->CSSetShaderResources(0, 1, &nullSRV);
	context->CSSetShaderResources(1, 1, &nullSRV);
	context->CSSetShaderResources(2, 1, &nullSRV);
}
