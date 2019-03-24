#include "GPUServiceCentre.h"
#include "GPUMessenger.h"

GPUMessenger::GPUMessenger(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
                           AthruGPU::GPUMemory& gpuMem)
{
	// Construct the system buffer
    sysBuf.InitBuf(device, gpuMem, SceneStuff::BODIES_PER_SYSTEM);

	// Cache a reference to Athru's CPU memory allocator; needed because D3D12's map/unmap logic expects double-pointers
    // to CPU access variables, and its much easier to make a smaller allocation for each mapping point (+ pass an address
    // to the result) than to try to recover offsets from the class layout + the instance pointer [this]
    // For per-frame maps/unmaps I'd normally create a single-element array, but those become invalid when resources are
    // mapped past the end of the calling scope (Athru keeps constant buffers mapped until the application closes)
    //StackAllocator* allocator = AthruCore::Utility::AccessMemory();

    // Construct the generic GPU input buffer
    //gpuInput = (GPUInput*)allocator->AlignedAlloc(sizeof(GPUInput), (uByte)std::alignment_of<GPUInput>(), false);
	gpuInputBuffer.InitCBuf(device,
                            gpuMem,
                            (address*)&gpuInput);

    // Construct the rendering-specific GPU input buffer
    //rndrInput = (RenderInput*)allocator->AlignedAlloc(sizeof(RenderInput), (uByte)std::alignment_of<RenderInput>(), false);
    renderInputBuffer.InitCBuf(device,
                               gpuMem,
                               (address*)&rndrInput);
}

GPUMessenger::~GPUMessenger()
{
    // Try to guarantee input buffers are unmapped on application exit
    gpuInputBuffer.~AthruResrc();
    renderInputBuffer.~AthruResrc();
}

void GPUMessenger::SysToGPU(SceneFigure::Figure* sceneFigures)
{
    SceneFigure::Figure* fig;
    D3D12_RANGE mapRange = { 0, 0 };
	assert(SUCCEEDED(sysBuf.resrc->Map(0, &mapRange, (void**)&fig)));
	memcpy(fig, sceneFigures, SceneStuff::BODIES_PER_SYSTEM);
    D3D12_RANGE unmapRange = { 0, sizeof(sceneFigures) };
	sysBuf.resrc->Unmap(0, &unmapRange);
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
    rndrInput->cameraPos = camera->GetTranslation();
    rndrInput->viewMat = viewMat;
    rndrInput->iViewMat = DirectX::XMMatrixInverse(&det,
                                                   viewMat);
    rndrInput->resInfo = DirectX::XMUINT4(GraphicsStuff::DISPLAY_WIDTH,
                                          GraphicsStuff::DISPLAY_HEIGHT,
                                          GraphicsStuff::NUM_AA_SAMPLES,
                                          GraphicsStuff::DISPLAY_AREA);
    rndrInput->tilingInfo = DirectX::XMUINT4(GraphicsStuff::TILING_WIDTH,
                                             GraphicsStuff::TILING_HEIGHT,
                                             GraphicsStuff::TILING_AREA,
                                             0);
    rndrInput->tileInfo = DirectX::XMUINT4(GraphicsStuff::TILE_WIDTH,
                                           GraphicsStuff::TILE_HEIGHT,
                                           GraphicsStuff::TILE_AREA,
                                           0);

    // Update physics & ecosystem inputs here...
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
