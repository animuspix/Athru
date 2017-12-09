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

void RenderManager::Render(Camera* mainCamera)
{
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
}

void RenderManager::RenderScene(Camera* mainCamera)
{
	// Path-trace the scene
	pathTracer->Dispatch(d3dContext,
						 mainCamera->GetTranslation(),
						 mainCamera->GetViewMatrix());

	// Apply post-processing to the render pass evaluated during the current
	// frame
	postProcessor->Dispatch(d3dContext,
							pathTracer->GetGICalcBufferReadable());
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
