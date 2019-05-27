#include "GPUServiceCentre.h"
#include "GPUMessenger.h"

GPUMessenger::GPUMessenger(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
                           AthruGPU::GPUMemory& gpuMem)
{
	// Initialize the GPU message buffer
	msgBuf.InitMsgBuf(device, gpuMem, (address*)&gpuInput);

	// Initialize the readback buffer
	rdbkBuf.InitReadbkBuf(device, gpuMem);

	// Initialize the system buffer
	sysBuf.InitBuf(device, gpuMem, SceneStuff::ALIGNED_PARAMETRIC_FIGURES_PER_SYSTEM, DXGI_FORMAT_UNKNOWN);

	// Create messaging command list + allocator
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
								   __uuidof(msgAlloc),
								   (void**)&msgAlloc);
	device->CreateCommandList(0x1,
							  D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
							  msgAlloc.Get(),
							  nullptr,
							  __uuidof(msgCmds),
							  (void**)&msgCmds);
}

GPUMessenger::~GPUMessenger()
{
    // Try to guarantee input buffers are unmapped on application exit
    msgBuf.~AthruResrc();
	sysBuf.~AthruResrc();
}

void GPUMessenger::SysToGPU(SceneFigure::Figure* sceneFigures)
{
	// Copy the given system to the upload heap
	u2Byte numBytes = SceneStuff::BODIES_PER_SYSTEM * sizeof(SceneFigure);
	memcpy(gpuInput + AthruGPU::EXPECTED_GPU_CONSTANT_MEM, sceneFigures, numBytes);

	// Copy from the upload heap into [sysBuf]
	D3D12_RESOURCE_BARRIER barriers[2] = { AthruGPU::TransitionBarrier(D3D12_RESOURCE_STATE_COPY_SOURCE, msgBuf.resrc, msgBuf.resrcState),
										   AthruGPU::TransitionBarrier(D3D12_RESOURCE_STATE_COPY_DEST, sysBuf.resrc, sysBuf.resrcState) };
	//msgCmds->ResourceBarrier(2, barriers); // Transition to copy resources
	msgCmds->CopyBufferRegion(sysBuf.resrc.Get(), 0, msgBuf.resrc.Get(), AthruGPU::EXPECTED_GPU_CONSTANT_MEM, numBytes); // Perform copy
	barriers[0] = AthruGPU::TransitionBarrier(msgBuf.resrcState, msgBuf.resrc, D3D12_RESOURCE_STATE_COPY_SOURCE);
	barriers[1] = AthruGPU::TransitionBarrier(sysBuf.resrcState, sysBuf.resrc, D3D12_RESOURCE_STATE_COPY_DEST);
	//msgCmds->ResourceBarrier(2, barriers); // Transition to previous resource states
	HRESULT hr = msgCmds->Close();
	assert(SUCCEEDED(hr));
	AthruGPU::GPU::AccessD3D()->GetGraphicsQueue()->ExecuteCommandLists(0, (ID3D12CommandList**)msgCmds.GetAddressOf());
	hr = msgAlloc->Reset();
	assert(SUCCEEDED(hr));
}

void GPUMessenger::InputsToGPU(const DirectX::XMFLOAT4& sysOri,
                               const Camera* camera)
{
    // Update basic GPU inputs
	std::chrono::nanoseconds currTimeNanoSecs = std::chrono::steady_clock::now().time_since_epoch();
	u4Byte currTime = std::chrono::duration_cast<std::chrono::duration<u4Byte>>(currTimeNanoSecs).count();
	gpuInput->tInfo.x = TimeStuff::deltaTime();
	gpuInput->tInfo.y = TimeStuff::time();
	gpuInput->tInfo.z = (float)TimeStuff::frameCtr;
	gpuInput->tInfo.w = 0.0f;
	gpuInput->systemOri = sysOri;

    // Update rendering inputs
    DirectX::XMMATRIX viewMat = camera->GetViewMatrix();
    DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(viewMat);
    gpuInput->cameraPos = camera->GetTranslation();
    gpuInput->viewMat = viewMat;
    gpuInput->iViewMat = DirectX::XMMatrixInverse(&det,
                                                  viewMat);
    gpuInput->resInfo = DirectX::XMUINT4(GraphicsStuff::DISPLAY_WIDTH,
                                         GraphicsStuff::DISPLAY_HEIGHT,
                                         GraphicsStuff::NUM_AA_SAMPLES,
                                         GraphicsStuff::DISPLAY_AREA);
    gpuInput->tilingInfo = DirectX::XMUINT4(GraphicsStuff::TILING_WIDTH,
                                            GraphicsStuff::TILING_HEIGHT,
                                            GraphicsStuff::TILING_AREA,
                                            0);
    gpuInput->tileInfo = DirectX::XMUINT4(GraphicsStuff::TILE_WIDTH,
                                          GraphicsStuff::TILE_HEIGHT,
                                          GraphicsStuff::TILE_AREA,
                                          0);

    // Update physics & ecosystem inputs here...
}

const Microsoft::WRL::ComPtr<ID3D12Resource>& GPUMessenger::AccessReadbackBuf()
{
	return rdbkBuf.resrc;
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
