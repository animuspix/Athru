#include "GPUServiceCentre.h"
#include "GPUMessenger.h"

GPUMessenger::GPUMessenger(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
                           AthruGPU::GPUMemory& gpuMem) : 
						   resrcContext(std::make_tuple(std::function<void()>([device, gpuMem, this]() { sysBuf.InitBuf(device, const_cast<AthruGPU::GPUMemory&>(gpuMem), SceneStuff::BODIES_PER_SYSTEM); }), 
														std::function<void()>([device, gpuMem, this]() { gpuInputBuffer.InitCBuf(device, const_cast<AthruGPU::GPUMemory&>(gpuMem), (address*)&gpuInput); })), 
														AthruGPU::RESRC_CTX::GENERIC, true) {}

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

std::function<void(const Microsoft::WRL::ComPtr<ID3D12Device>&, AthruGPU::GPUMemory&)> GPUMessenger::RenderInputInitter()
{
	decltype(renderInputBuffer)* renderInputBuf = &renderInputBuffer;
	RenderInput*& renderInput = rndrInput;
	return std::function([renderInputBuf, renderInput](const Microsoft::WRL::ComPtr<ID3D12Device>& device,
													   AthruGPU::GPUMemory& gpuMem) { renderInputBuf->InitCBuf(device, gpuMem, (address*)&renderInput); });
}

AthruGPU::ResrcContext<std::function<void()>, std::function<void()>>& GPUMessenger::AccessResrcContext()
{
	return resrcContext;
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
