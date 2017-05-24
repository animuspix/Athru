#include "UtilityServiceCentre.h"
#include "SystemSampler.h"

SystemSampler::SystemSampler(ID3D11Device* device, HWND windowHandle,
							 LPCWSTR shaderFilePath,
							 ID3D11UnorderedAccessView* writableColorShaderResource,
							 ID3D11UnorderedAccessView* sceneNormalShaderResource,
							 ID3D11UnorderedAccessView* scenePBRShaderResource,
							 ID3D11UnorderedAccessView* sceneEmissivityShaderResource) :
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

	// Setup the system-data buffer description
	D3D11_BUFFER_DESC systemBufferDesc;
	systemBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	systemBufferDesc.ByteWidth = sizeof(SystemBuffer);
	systemBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	systemBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	systemBufferDesc.MiscFlags = 0;
	systemBufferDesc.StructureByteStride = 0;

	// Create a pointer to the system-data buffer so we can easily access it from the
	// CPU before starting a draw call
	result = device->CreateBuffer(&systemBufferDesc, NULL, &systemBufferPttr);
	assert(SUCCEEDED(result));
}

SystemSampler::~SystemSampler()
{
	// Release the system-data-buffer
	systemBufferPttr->Release();
	systemBufferPttr = nullptr;
}

void SystemSampler::Dispatch(ID3D11DeviceContext* context,
						     DirectX::XMVECTOR cameraPosition, DirectX::XMVECTOR starPosition,
							 DirectX::XMFLOAT4 localStarColor, DirectX::XMVECTOR planetAPosition,
							 DirectX::XMVECTOR planetBPosition, DirectX::XMVECTOR planetCPosition,
							 DirectX::XMFLOAT4 planetAColor, DirectX::XMFLOAT4 planetBColor,
							 DirectX::XMFLOAT4 planetCColor)
{
	// Long integer used for storing success/failure for different
	// DirectX operations
	HRESULT result;

	// Define the shader object associated with [this] as the current compute
	// shader on the GPU
	context->CSSetShader(shader, 0, 0);

	// Pass the writable shader resources associated with [this] onto the GPU
	context->CSSetUnorderedAccessViews(0, 1, &sceneColorTexture, 0);
	context->CSSetUnorderedAccessViews(1, 1, &sceneNormalTexture, 0);
	context->CSSetUnorderedAccessViews(2, 1, &scenePBRTexture, 0);
	context->CSSetUnorderedAccessViews(3, 1, &sceneEmissivityTexture, 0);

	// Define the given per-dispatch shader constants on the GPU

	// Make the system-data buffer writable by mapping it's data onto a local variable
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	result = context->Map(systemBufferPttr, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	assert(SUCCEEDED(result));

	// Cast the raw data mapped onto [mappedResource] into a pointer formatted
	// with it's original type (SystemBuffer) which can be used to edit the
	// data itself
	SystemBuffer* systemBufferData = (SystemBuffer*)mappedResource.pData;

	// Write system data to the system-data-buffer via [systemBufferData]

	// System sampling is all done in voxel-grid coordinates, so just set the
	// camera-position constant to a fixed point in the centre of the grid
	// (the camera position is expected to be locked relative to the sampling
	// area, so there isn't much value in setting it to anything else)
	systemBufferData->cameraPos = DirectX::XMFLOAT4((float)(GraphicsStuff::VOXEL_GRID_WIDTH / 2),
													(float)(GraphicsStuff::VOXEL_GRID_WIDTH / 2),
													(float)(GraphicsStuff::VOXEL_GRID_WIDTH / 2), 1);

	// Fixing the camera position means treating it like the system origin,
	// so all the positions below are slightly adjusted to match the new
	// coordinate system before they enter the shader
	systemBufferData->starPos = ClipToViewableSpace(_mm_sub_ps(starPosition, cameraPosition));
	systemBufferData->starColor = localStarColor;
	systemBufferData->planetAPos = ClipToViewableSpace(_mm_sub_ps(planetAPosition, cameraPosition));
	systemBufferData->planetBPos = ClipToViewableSpace(_mm_sub_ps(planetBPosition, cameraPosition));
	systemBufferData->planetCPos = ClipToViewableSpace(_mm_sub_ps(planetCPosition, cameraPosition));
	systemBufferData->planetAColor = planetAColor;
	systemBufferData->planetBColor = planetBColor;
	systemBufferData->planetCColor = planetCColor;

	// Discard the mapping between the GPU-side [effectStatusBufferPttr] and the CPU-side
	// [mappedResource]
	context->Unmap(systemBufferPttr, 0);

	// Update the shader object with the edited system-data buffer
	context->CSSetConstantBuffers(0, 1, &systemBufferPttr);

	// Begin processing the shader
	context->Dispatch(GraphicsStuff::VOXEL_GRID_WIDTH, GraphicsStuff::VOXEL_GRID_WIDTH, GraphicsStuff::VOXEL_GRID_WIDTH);

	// Resources can't be bound in more than one place at once, so unbind
	// them here
	ID3D11UnorderedAccessView* nullUAV = nullptr;
	context->CSSetUnorderedAccessViews(0, 1, &nullUAV, 0);
	context->CSSetUnorderedAccessViews(1, 1, &nullUAV, 0);
	context->CSSetUnorderedAccessViews(2, 1, &nullUAV, 0);
	context->CSSetUnorderedAccessViews(3, 1, &nullUAV, 0);
}
