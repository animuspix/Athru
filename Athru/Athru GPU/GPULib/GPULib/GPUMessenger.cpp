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
	const Microsoft::WRL::ComPtr<ID3D11Device>& device = AthruGPU::GPUServiceCentre::AccessD3D()->GetDevice();
	HRESULT result = device->CreateBuffer(&figBufferInputDesc, NULL, &figBufferInput);
	assert(SUCCEEDED(result));

	// Create a shader-resource-view of the GPU-read-only buffer created above
	result = device->CreateShaderResourceView(figBufferInput.Get(), &figBufferInputViewDesc, &gpuReadableSceneView);
	assert(SUCCEEDED(result));

	// Describe + build the output buffer we'll be using
	// to store changes applied to the figure
	// data while it was on the GPU
	GPGPUStuff::BuildRWStructBuffer<SceneFigure::Figure>(device,
													     figBufferOutput,
													     nullptr,
													     gpuWritableSceneView,
														 (D3D11_BUFFER_UAV_FLAG)0,
													     SceneStuff::MAX_NUM_SCENE_FIGURES);

	// Describe + build the staging buffer we'll be using
	// to transfer data from the GPU output buffer back
	// across to the CPU
	GPGPUStuff::BuildStagingStructBuffer<SceneFigure::Figure>(device,
															  figBufferStaging,
															  SceneStuff::MAX_NUM_SCENE_FIGURES);
}

GPUMessenger::~GPUMessenger() {}

void GPUMessenger::FrameStartSync(SceneFigure* sceneFigures)
{
	// Write to each individual index within the read-only (for the GPU) scene data buffer
	const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context = AthruGPU::GPUServiceCentre::AccessD3D()->GetDeviceContext();
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT result = context->Map(figBufferInput.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
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
	context->Unmap(figBufferInput.Get(), 0);

	// Pass the figure-input-buffer onto the GPU
	context->CSSetShaderResources(0, 1, gpuReadableSceneView.GetAddressOf());
}

void GPUMessenger::FrameEndSync()
{
	// Pass the changes made to the data on the GPU into the intermediate CPU-read-only buffer
	const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context = AthruGPU::GPUServiceCentre::AccessD3D()->GetDeviceContext();
	context->CopyResource(figBufferStaging.Get(), figBufferOutput.Get());

	// Map the data in the staging buffer back into a local [void*]
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT result = context->Map(figBufferStaging.Get(), 0, D3D11_MAP_READ, 0, &mappedResource);
	assert(SUCCEEDED(result));

	// Access + cast the [void*] reference we cached above
	SceneFigure::Figure* gpuFigures = (SceneFigure::Figure*)mappedResource.pData;

	// Copy the data for each figure back onto the CPU
	for (byteUnsigned i = 0; i < SceneStuff::MAX_NUM_SCENE_FIGURES; i += 1)
	{
		// Extract the address of the original object associated with the current
		// figure
		MemoryStuff::addrValType addressLO = ((MemoryStuff::addrValType)gpuFigures[i].origin.x) << (MemoryStuff::halfAddrLength());
		MemoryStuff::addrValType addressHI = ((MemoryStuff::addrValType)gpuFigures[i].origin.y);
		SceneFigure* addressFull = (SceneFigure*)(addressLO | addressHI);

		// Copy in the data for the current figure
		addressFull->SetCoreFigure(gpuFigures[i]);
	}

	// Break the connection to the GPU-side scene-data-buffer
	context->Unmap(figBufferStaging.Get(), 0);
}

const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& GPUMessenger::GetGPUReadableSceneView()
{
	return gpuReadableSceneView;
}

const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& GPUMessenger::GetGPUWritableSceneView()
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
