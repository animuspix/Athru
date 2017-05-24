#include "UtilityServiceCentre.h"
#include "PlanetSampler.h"

PlanetSampler::PlanetSampler(ID3D11Device* device, HWND windowHandle,
							 LPCWSTR shaderFilePath,
							 ID3D11ShaderResourceView* upperHeightfield,
							 ID3D11ShaderResourceView* lowerHeightfield,
							 ID3D11UnorderedAccessView* writableColorShaderResource,
							 ID3D11UnorderedAccessView* sceneNormalShaderResource,
							 ID3D11UnorderedAccessView* scenePBRShaderResource,
							 ID3D11UnorderedAccessView* sceneEmissivityShaderResource) :
							 upperTectoTexture(upperHeightfield),
							 lowerTectoTexture(lowerHeightfield),
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

	// Setup the planetary buffer description
	D3D11_BUFFER_DESC planetBufferDesc;
	planetBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	planetBufferDesc.ByteWidth = sizeof(PlanetBuffer);
	planetBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	planetBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	planetBufferDesc.MiscFlags = 0;
	planetBufferDesc.StructureByteStride = 0;

	// Create a pointer to the planetary buffer so we can easily access it from the
	// CPU before starting a draw call
	result = device->CreateBuffer(&planetBufferDesc, NULL, &planetBufferPttr);
	assert(SUCCEEDED(result));
}

PlanetSampler::~PlanetSampler()
{
	// Release the planetary buffer
	planetBufferPttr->Release();
	planetBufferPttr = nullptr;
}

void PlanetSampler::Dispatch(ID3D11DeviceContext* context, DirectX::XMVECTOR planetPosition,
							 DirectX::XMVECTOR cameraPosition, DirectX::XMFLOAT4 avgPlanetColor,
							 DirectX::XMVECTOR starPosition)
{
	// Long integer used for storing success/failure for different
	// DirectX operations
	HRESULT result;

	// Define the shader object associated with [this] as the current compute
	// shader on the GPU
	context->CSSetShader(shader, 0, 0);

	// Pass the read-only shader resources associated with [this] onto the GPU
	context->CSSetShaderResources(0, 1, &upperTectoTexture);
	context->CSSetShaderResources(1, 1, &lowerTectoTexture);

	// Pass the writable shader resources associated with [this] onto the GPU
	context->CSSetUnorderedAccessViews(0, 1, &sceneColorTexture, 0);
	context->CSSetUnorderedAccessViews(1, 1, &sceneNormalTexture, 0);
	context->CSSetUnorderedAccessViews(2, 1, &scenePBRTexture, 0);
	context->CSSetUnorderedAccessViews(3, 1, &sceneEmissivityTexture, 0);

	// Define the given per-dispatch shader constants on the GPU

	// Make the planetary buffer writable by mapping it's data onto a local variable
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	result = context->Map(planetBufferPttr, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	assert(SUCCEEDED(result));

	// Cast the raw data mapped onto [mappedResource] into a pointer formatted
	// with it's original type (PlanetBuffer) which can be used to edit the
	// data itself
	PlanetBuffer* planetBufferData = (PlanetBuffer*)mappedResource.pData;

	// Write system data to the planetary buffer via [planetBufferData]
	planetBufferData->cameraSurfacePos = MathsStuff::sseToScalarVector(_mm_sub_ps(cameraPosition, planetPosition));
	planetBufferData->planetAvgColor = avgPlanetColor;
	planetBufferData->starPos = MathsStuff::sseToScalarVector(_mm_sub_ps(starPosition, cameraPosition));

	// Discard the mapping between the GPU-side [planetBufferPttr] and the CPU-side
	// [mappedResource]
	context->Unmap(planetBufferPttr, 0);

	// Update the shader object with the edited planetary buffer
	context->CSSetConstantBuffers(0, 1, &planetBufferPttr);

	// Begin processing the shader
	context->Dispatch(GraphicsStuff::VOXEL_GRID_WIDTH, GraphicsStuff::VOXEL_GRID_WIDTH, GraphicsStuff::VOXEL_GRID_WIDTH);

	// Resources can't be bound in more than one place at once, so unbind
	// them here
	ID3D11ShaderResourceView* nullSRV = nullptr;
	context->CSSetShaderResources(0, 1, &nullSRV);
	context->CSSetShaderResources(1, 1, &nullSRV);

	ID3D11UnorderedAccessView* nullUAV = nullptr;
	context->CSSetUnorderedAccessViews(0, 1, &nullUAV, 0);
	context->CSSetUnorderedAccessViews(1, 1, &nullUAV, 0);
	context->CSSetUnorderedAccessViews(2, 1, &nullUAV, 0);
	context->CSSetUnorderedAccessViews(3, 1, &nullUAV, 0);
}
