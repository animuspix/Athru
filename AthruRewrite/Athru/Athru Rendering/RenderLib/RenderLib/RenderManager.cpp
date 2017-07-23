// Athru utility services
#include "UtilityServiceCentre.h"

// Athru rendering services
#include "GPUServiceCentre.h"

// The Direct3D handler class
#include "Direct3D.h"

// Pipeline shaders
#include "PostProcessor.h"

// The scene texture shader
#include "RayMarcher.h"

// Generic objects relevant to [this]
#include "Camera.h"

// The header containing declarations used by [this]
#include "RenderManager.h"

RenderManager::RenderManager(ID3D11ShaderResourceView* postProcessShaderResource)
{
	// Cache a class-scope reference to the Direct3D handler class
	d3D = AthruGPU::GPUServiceCentre::AccessD3D();

	// Cache a class-scope reference to the Direct3D rendering context
	d3dContext = d3D->GetDeviceContext();

	// Cache a local reference to the Direct3D rendering device
	ID3D11Device* d3dDevice = d3D->GetDevice();

	// Cache a local copy of the window handle
	HWND localWindowHandle = AthruUtilities::UtilityServiceCentre::AccessApp()->GetHWND();

	// Construct the shaders used by [this]

	// Construct the raymarching shader + the post-processing shader
	rayMarcher = new RayMarcher(L"SceneVis.cso");
	postProcessor = new PostProcessor(d3dDevice, localWindowHandle, L"PostVertPlotter.cso", L"PostColorizer.cso",
									  postProcessShaderResource);
}

RenderManager::~RenderManager()
{
	// Delete heap-allocated data within the ray-marching shader
	rayMarcher->~RayMarcher();
	rayMarcher = nullptr;

	// Delete heap-allocated data within the post-processing shader
	postProcessor->~PostProcessor();
	postProcessor = nullptr;
}

void RenderManager::Render(Camera* mainCamera,
						   ID3D11ShaderResourceView* gpuReadableSceneDataView,
						   ID3D11UnorderedAccessView* gpuWritableSceneDataView,
						   fourByteUnsigned numSceneFigures)
{
	// Cache a local reference to the camera's viewfinder
	// (screen rect)
	ScreenRect* viewFinderPttr = mainCamera->GetViewFinder();

	// Prepare DirectX for scene rendering
	d3D->BeginScene();

	// Render scene to the screen texture
	this->RenderScene(mainCamera,
					  gpuReadableSceneDataView,
					  gpuWritableSceneDataView,
					  numSceneFigures);

	// Post-process
	this->PostProcess(viewFinderPttr, d3D->GetWorldMatrix(), mainCamera->GetViewMatrix(), d3D->GetPerspProjector());

	// Present the post-processed scene to the display
	d3D->EndScene();
}

void RenderManager::RenderScene(Camera* mainCamera,
								ID3D11ShaderResourceView* gpuReadableSceneDataView,
								ID3D11UnorderedAccessView* gpuWritableSceneDataView,
								fourByteUnsigned numSceneFigures)
{
	// Pre-fill the scene texture with the forms defined in the given
	// distance functions
	rayMarcher->Dispatch(d3dContext,
						 mainCamera->GetTranslation(),
						 mainCamera->GetViewMatrix(),
						 gpuReadableSceneDataView,
						 gpuWritableSceneDataView,
						 numSceneFigures);
}

void RenderManager::PostProcess(ScreenRect* screenRect,
								DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
{
	// Pass the rect onto the GPU, then render it with the post-processing shader
	screenRect->PassToGPU(d3dContext);
	postProcessor->Render(d3dContext, false, false, false);
}

Direct3D* RenderManager::GetD3D()
{
	return d3D;
}

// Push constructions for this class through Athru's custom allocator
void* RenderManager::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<RenderManager>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void RenderManager::operator delete(void* target)
{
	return;
}
