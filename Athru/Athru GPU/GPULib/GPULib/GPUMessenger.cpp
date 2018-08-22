#include "GPUServiceCentre.h"
#include "GPUMessenger.h"

GPUMessenger::GPUMessenger(const Microsoft::WRL::ComPtr<ID3D11Device>& device)
{
	// Construct the GPU scene buffer
	sceneBuf = AthruBuffer<SceneFigure::Figure, GPGPUStuff::StrmBuffer>(device,
																		nullptr,
																		SceneStuff::MAX_NUM_SCENE_FIGURES);
}

GPUMessenger::~GPUMessenger() {}

void GPUMessenger::SceneToGPU(const Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
							  SceneFigure::Figure* sceneFigures)
{
	// Write to each individual index within the read-only (for the GPU) scene-buffer
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT result = context->Map(sceneBuf.buf.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	assert(SUCCEEDED(result));
	for (fourByteUnsigned i = 0; i < SceneStuff::MAX_NUM_SCENE_FIGURES; i += 1)
	{
		// Get a pointer to the data referenced by [mappedResource]
		SceneFigure::Figure* dataPttr;
		dataPttr = (SceneFigure::Figure*)mappedResource.pData;

		// Copy in the data for the current figure
		dataPttr[i] = sceneFigures[i];
	}

	// Break the write-allowed connection to the GPU-side scene-buffer
	context->Unmap(sceneBuf.buf.Get(), 0);

	// Pass the scene-buffer onto the GPU
	context->CSSetShaderResources(0, 1, sceneBuf.view().GetAddressOf());

	// Pass particle data (for physics/atmosphere simulation) along here...
}

const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& GPUMessenger::GetGPUSceneView()
{
	return sceneBuf.view();
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
