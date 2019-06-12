#include "GPUServiceCentre.h"
#include "GPUMessenger.h"

GPUMessenger::GPUMessenger(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
                           AthruGPU::GPUMemory& gpuMem) : gpuReadable(nullptr)
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

	// Record upload -> dedicated memory copy
	// (used to transfer parametric figure memory onto the GPU in each new system)
	D3D12_RESOURCE_BARRIER sysBufBarriers[2] = { AthruGPU::TransitionBarrier(D3D12_RESOURCE_STATE_COPY_DEST, sysBuf.resrc, sysBuf.resrcState),
												 AthruGPU::TransitionBarrier(sysBuf.resrcState, sysBuf.resrc, D3D12_RESOURCE_STATE_COPY_DEST) };
	msgCmds->ResourceBarrier(1, sysBufBarriers);
	msgCmds->CopyBufferRegion(sysBuf.resrc.Get(), 0, msgBuf.resrc.Get(), 0, sysBytes); // Perform copy
	msgCmds->ResourceBarrier(1, sysBufBarriers + 1);
	HRESULT hr = msgCmds->Close();
	assert(SUCCEEDED(hr));
}

GPUMessenger::~GPUMessenger()
{
	// Unmap message buffer, explicitly clear smart pointers
	D3D12_RANGE range;
	range.Begin = 0;
	range.End = sysBytes;
	msgBuf.resrc->Unmap(0, &range);
	msgBuf.resrc = nullptr;
	rdbkBuf.resrc = nullptr;
	sysBuf.resrc = nullptr;
	msgAlloc = nullptr;
	msgCmds = nullptr;
}

void GPUMessenger::SysToGPU(SceneFigure::Figure* sceneFigures)
{
	// Copy the given system to the upload heap
	memcpy(gpuInput, sceneFigures, sysBytes);

	// Copy from the upload heap into [sysBuf]
	// Scenes aren't configured for uploading complete systems yet, so ignore this for now
	Direct3D* d3d = AthruGPU::GPU::AccessD3D();
	d3d->GetGraphicsQueue()->ExecuteCommandLists(1, (ID3D12CommandList**)msgCmds.GetAddressOf());
	d3d->WaitForQueue<D3D12_COMMAND_LIST_TYPE_DIRECT>();
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
	gpuInput->bounceInfo = DirectX::XMFLOAT4(GraphicsStuff::MAX_NUM_BOUNCES, GraphicsStuff::NUM_SUPPORTED_SURF_BXDFS, GraphicsStuff::EPSILON_MAX, 0.0f);
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
