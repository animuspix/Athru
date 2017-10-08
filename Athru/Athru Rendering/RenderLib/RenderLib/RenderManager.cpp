// Athru utility services
#include "UtilityServiceCentre.h"

// Athru rendering services
#include "GPUServiceCentre.h"

// The Direct3D handler class
#include "Direct3D.h"

// The presentation shader
#include "ScreenPainter.h"

// The scene path-tracer
#include "PathTracer.h"

// The post-processing shader
#include "PostProcessor.h"

// The scene camera
#include "Camera.h"

// CPU-side figure property declarations + methods
#include "SceneFigure.h"

// The header containing declarations used by [this]
#include "RenderManager.h"

RenderManager::RenderManager()
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

	// Construct the path tracer and the image presentation shader
	pathTracer = new PathTracer(L"SceneVis.cso");
	postProcessor = new PostProcessor(L"PostProcessor.cso");
	screenPainter = new ScreenPainter(d3dDevice, localWindowHandle,
									  L"PresentationVerts.cso", L"PresentationColors.cso");

	// Pass useful buffers, textures, etc. onto the GPU
	ID3D11UnorderedAccessView* displayTexture = AthruGPU::GPUServiceCentre::AccessTextureManager()->GetDisplayTexture(AVAILABLE_DISPLAY_TEXTURES::SCREEN_TEXTURE).asWritableShaderResource;
	ID3D11UnorderedAccessView* gpuRandView = AthruGPU::GPUServiceCentre::AccessGPURandView();
	ID3D11ShaderResourceView* gpuReadableSceneDataView = AthruGPU::GPUServiceCentre::AccessGPUMessenger()->GetGPUReadableSceneView();
	ID3D11UnorderedAccessView* gpuWritableSceneDataView = AthruGPU::GPUServiceCentre::AccessGPUMessenger()->GetGPUWritableSceneView();

	d3dContext->CSSetShaderResources(0, 1, &gpuReadableSceneDataView);
	d3dContext->CSSetUnorderedAccessViews(0, 1, &gpuWritableSceneDataView, 0);
	d3dContext->CSSetUnorderedAccessViews(1, 1, &gpuRandView, 0);
	d3dContext->CSSetUnorderedAccessViews(2, 1, &displayTexture, 0);
}

RenderManager::~RenderManager()
{
	// Delete heap-allocated data within the path-tracer
	pathTracer->~PathTracer();
	pathTracer = nullptr;

	// Delete heap-allocated data within the post-processing shader
	postProcessor->~PostProcessor();
	postProcessor = nullptr;

	// Delete heap-allocated data within the presentation shader
	screenPainter->~ScreenPainter();
	screenPainter = nullptr;
}

void RenderManager::Render(Camera* mainCamera,
						   SceneFigure* localSceneFigures)
{
	// Pass CPU figures to the GPU for rendering
	// Much better ways to do this, fix when possible
	AthruGPU::GPUServiceCentre::AccessGPUMessenger()->FrameStartSync();

	// Cache a local reference to the camera's viewfinder
	// (screen rect)
	ScreenRect* viewFinderPttr = mainCamera->GetViewFinder();

	// Prepare DirectX for scene rendering
	d3D->BeginScene();

	// Render scene to the screen texture
	this->RenderScene(mainCamera);

	// Paint the rendered image onto a
	// rectangle and draw the result to the
	// screen
	this->Display(viewFinderPttr);

	// Present the post-processed scene to the display
	d3D->EndScene();

	// No separate GPU update yet, so do that here for now
	// Commented out because no actual updates are happening atm, and
	// syncing the ([null], because no updates) GPU data with the CPU
	// will wipe the figures :P
	//AthruGPU::GPUServiceCentre::AccessGPUMessenger()->FrameEndSync();
}

void RenderManager::RenderScene(Camera* mainCamera)
{
	// Path-trace the scene
	// Also pass in a shader-friendly view of the GPU RNG state buffer so we
	// can use the Xorshift random number generator as a noise source for colors/textures,
	// planetary/asteroid terrain, etc.
	pathTracer->Dispatch(d3dContext,
						 mainCamera->GetTranslation(),
						 mainCamera->GetViewMatrix());

	// Apply post-processing to the render pass evaluated during the current
	// frame
	// As above, all the values needed for post-processing were already loaded
	// during ray-marching
	postProcessor->Dispatch(d3dContext, pathTracer->GetGICalcBufferReadable());
}

void RenderManager::Display(ScreenRect* screenRect)
{
	// Pass the rect onto the GPU, then render it with the post-processing shader
	screenRect->PassToGPU(d3dContext);
	screenPainter->Render(d3dContext);
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
