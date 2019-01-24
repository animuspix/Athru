#include "GPUServiceCentre.h"
#include "GPUMessenger.h"

GPUMessenger::GPUMessenger(const Microsoft::WRL::ComPtr<ID3D11Device>& device)
{
	// Construct the GPU scene buffer
	sceneBuf = AthruGPU::AthruBuffer<SceneFigure::Figure, AthruGPU::StrmBuffer>(device,
																		nullptr,
																		SceneStuff::MAX_NUM_SCENE_FIGURES);
	// Construct the GPU generic input buffer
	gpuInputBuffer = AthruGPU::AthruBuffer<GPUInput, AthruGPU::CBuffer>(device, nullptr);
}

GPUMessenger::~GPUMessenger() {}

void GPUMessenger::SceneToGPU(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
							  SceneFigure::Figure* sceneFigures)
{
	// Write to each individual index within the read-only (for the GPU) scene-buffer
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT result = context->Map(sceneBuf.buf.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	assert(SUCCEEDED(result));
	for (u4Byte i = 0; i < SceneStuff::PLANETS_PER_SYSTEM; i += 1) // No plant/animal definitions atm
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

void GPUMessenger::CoreInputToGPU(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
								  const DirectX::XMFLOAT4& sysOri)
{
	D3D11_MAPPED_SUBRESOURCE shaderInput;
	context->Map(gpuInputBuffer.buf.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &shaderInput);
	GPUInput* shaderInputPtr = (GPUInput*)shaderInput.pData;
	std::chrono::nanoseconds currTimeNanoSecs = std::chrono::steady_clock::now().time_since_epoch();
	u4Byte currTime = std::chrono::duration_cast<std::chrono::duration<u4Byte>>(currTimeNanoSecs).count();
	shaderInputPtr->tInfo.x = TimeStuff::deltaTime();
	shaderInputPtr->tInfo.y = TimeStuff::time();
	shaderInputPtr->tInfo.z = (float)TimeStuff::frameCtr;
	shaderInputPtr->tInfo.w = 0.0f;
	shaderInputPtr->systemOri = sysOri;
	context->Unmap(gpuInputBuffer.buf.Get(), 0);
	context->CSSetConstantBuffers(0, 1, gpuInputBuffer.buf.GetAddressOf());
}

const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& GPUMessenger::GetGPUSceneView()
{
	return sceneBuf.view();
}

// Push constructions for this class through Athru's custom allocator
void* GPUMessenger::operator new(size_t size)
{
	StackAllocator* allocator = AthruCore::Utility::AccessMemory();
	return allocator->AlignedAlloc(size, (uByte)std::alignment_of<GPUMessenger>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void GPUMessenger::operator delete(void* target)
{
	return;
}
