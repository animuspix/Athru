#include "GPUServiceCentre.h"
#include "GPUMessenger.h"

GPUMessenger::GPUMessenger()
{
	// Describe the input buffer we'll be using
	// to pass CPU-side data across to the GPU
	D3D11_BUFFER_DESC figBufferInputDesc;
	figBufferInputDesc.Usage = D3D11_USAGE_DYNAMIC;
	figBufferInputDesc.ByteWidth = sizeof(SceneFigure::Figure) * SceneStuff::MAX_NUM_SCENE_FIGURES;
	figBufferInputDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	figBufferInputDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	figBufferInputDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	figBufferInputDesc.StructureByteStride = sizeof(SceneFigure::Figure);

	D3D11_BUFFER_SRV figBufferPayloadDesc;
	figBufferPayloadDesc.FirstElement = 0;
	figBufferPayloadDesc.NumElements = SceneStuff::MAX_NUM_SCENE_FIGURES;

	D3D11_SHADER_RESOURCE_VIEW_DESC figBufferInputViewDesc;
	figBufferInputViewDesc.Format = DXGI_FORMAT_UNKNOWN;
	figBufferInputViewDesc.Buffer = figBufferPayloadDesc;
	figBufferInputViewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;

	// Instantiate the read-only GPU figure buffer from the description
	// defined above
	ID3D11Device* device = AthruGPU::GPUServiceCentre::AccessD3D()->GetDevice();
	HRESULT result = device->CreateBuffer(&figBufferInputDesc, NULL, &figBufferInput);
	assert(SUCCEEDED(result));

	// Create a shader-resource-view of the GPU-read-only buffer created above
	result = device->CreateShaderResourceView(figBufferInput, &figBufferInputViewDesc, &(gpuReadableSceneView));
	assert(SUCCEEDED(result));

	// Describe the output buffer we'll be using
	// to store changes applied to the figure
	// data while it was on the GPU
	D3D11_BUFFER_DESC figBufferOuputDesc;
	figBufferOuputDesc.Usage = D3D11_USAGE_DEFAULT;
	figBufferOuputDesc.ByteWidth = sizeof(SceneFigure::Figure) * SceneStuff::MAX_NUM_SCENE_FIGURES;
	figBufferOuputDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	figBufferOuputDesc.CPUAccessFlags = 0;
	figBufferOuputDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	figBufferOuputDesc.StructureByteStride = sizeof(SceneFigure::Figure);

	D3D11_BUFFER_UAV figBufferDeltaDesc;
	figBufferDeltaDesc.FirstElement = 0;
	figBufferDeltaDesc.Flags = 0;
	figBufferDeltaDesc.NumElements = SceneStuff::MAX_NUM_SCENE_FIGURES;

	D3D11_UNORDERED_ACCESS_VIEW_DESC figBufferOutputViewDesc;
	figBufferOutputViewDesc.Format = DXGI_FORMAT_UNKNOWN;
	figBufferOutputViewDesc.Buffer = figBufferDeltaDesc;
	figBufferOutputViewDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;

	// Instantiate the write-allowed GPU figure buffer from the description
	// defined above
	result = device->CreateBuffer(&figBufferOuputDesc, NULL, &figBufferOutput);
	assert(SUCCEEDED(result));

	// Create an unordered-access-views of the GPU-write-allowed buffer created above
	result = device->CreateUnorderedAccessView(figBufferOutput, &figBufferOutputViewDesc, &(gpuWritableSceneView));
	assert(SUCCEEDED(result));

	// Describe the staging buffer we'll be using
	// to transfer data from the GPU output
	// buffer back across to the CPU
	D3D11_BUFFER_DESC figBufferStagingDesc;
	figBufferStagingDesc.Usage = D3D11_USAGE_STAGING;
	figBufferStagingDesc.ByteWidth = sizeof(SceneFigure::Figure) * SceneStuff::MAX_NUM_SCENE_FIGURES;
	figBufferStagingDesc.BindFlags = 0;
	figBufferStagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	figBufferStagingDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	figBufferStagingDesc.StructureByteStride = sizeof(SceneFigure::Figure);

	// Instantiate the staging buffer from the description created above
	result = device->CreateBuffer(&figBufferStagingDesc, NULL, &figBufferStaging);
	assert(SUCCEEDED(result));
}

GPUMessenger::~GPUMessenger()
{
	figBufferInput->Release();
	figBufferInput = nullptr;

	figBufferOutput->Release();
	figBufferOutput = nullptr;

	figBufferStaging->Release();
	figBufferStaging = nullptr;

	gpuReadableSceneView->Release();
	gpuReadableSceneView = nullptr;

	gpuWritableSceneView->Release();
	gpuWritableSceneView = nullptr;
}

void GPUMessenger::FrameStartSync(SceneFigure* sceneFigures)
{
	// Write to each individual index within the read-only (for the GPU) scene data buffer
	ID3D11DeviceContext* context = AthruGPU::GPUServiceCentre::AccessD3D()->GetDeviceContext();
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT result = context->Map(figBufferInput, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	assert(SUCCEEDED(result));

	for (fourByteUnsigned i = 0; i < SceneStuff::MAX_NUM_SCENE_FIGURES; i += 1)
	{
		// Get a pointer to the data referenced by [mappedResource]
		SceneFigure::Figure* dataPttr;
		dataPttr = (SceneFigure::Figure*)mappedResource.pData;

		// Copy in the data for the current figure
		dataPttr[i] = sceneFigures[i].GetCoreFigure();
	}

	// Break the write-allowed connection to the GPU-side scene-data-buffer
	context->Unmap(figBufferInput, 0);

	// Pass the figure-input-buffer onto the GPU
	context->CSSetShaderResources(0, 1, &gpuReadableSceneView);
}

void GPUMessenger::FrameEndSync()
{
	// Pass the changes made to the data on the GPU into the intermediate CPU-read-only buffer
	ID3D11DeviceContext* context = AthruGPU::GPUServiceCentre::AccessD3D()->GetDeviceContext();
	context->CopyResource(figBufferStaging, figBufferOutput);

	// Read the data pased into the staging buffer back into the relevant CPU-side [SceneFigure]s
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT result = context->Map(figBufferStaging, 0, D3D11_MAP_READ, 0, &mappedResource);
	assert(SUCCEEDED(result));

	for (byteUnsigned i = 0; i < SceneStuff::MAX_NUM_SCENE_FIGURES; i += 1)
	{
		// Get a pointer to the data referenced by [mappedResource]
		SceneFigure::Figure* dataPttr;
		dataPttr = (SceneFigure::Figure*)mappedResource.pData;

		// Extract the address of the original object associated with the current
		// figure
		eightByteUnsigned addressLO = ((eightByteUnsigned)dataPttr[i].origin.x) << 32;
		eightByteUnsigned addressHI = ((eightByteUnsigned)dataPttr[i].origin.y);
		SceneFigure* addressFull = (SceneFigure*)(addressLO | addressHI);

		// Copy in the data for the current figure
		addressFull->SetCoreFigure(dataPttr[i]);
	}

	// Break the connection to the GPU-side scene-data-buffer
	context->Unmap(figBufferStaging, 0);
}

ID3D11ShaderResourceView* GPUMessenger::GetGPUReadableSceneView()
{
	return gpuReadableSceneView;
}

ID3D11UnorderedAccessView* GPUMessenger::GetGPUWritableSceneView()
{
	return gpuWritableSceneView;
}

// Push constructions for this class through Athru's custom allocator
void* GPUMessenger::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<GPUMessenger>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void GPUMessenger::operator delete(void* target)
{
	return;
}

