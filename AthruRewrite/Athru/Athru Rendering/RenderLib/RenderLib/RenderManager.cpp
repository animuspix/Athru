// Athru utility services
#include "UtilityServiceCentre.h"

// Athru rendering services
#include "GPUServiceCentre.h"

// The Direct3D handler class
#include "Direct3D.h"

// The presentation shader
#include "ScreenPainter.h"

// The scene ray-marcher
#include "RayMarcher.h"

// The scene path tracer
// (needed for global illumination)
#include "PathTracer.h"

// The post-processing shader
#include "PostProcessor.h"

// The scene camera
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

	// Construct the raymarching shader, the path tracer, and the image presentation shader
	rayMarcher = new RayMarcher(L"SceneVis.cso");
	pathTracer = new PathTracer(L"PathTracer.cso");
	postProcessor = new PostProcessor(L"PostProcessor.cso");
	screenPainter = new ScreenPainter(d3dDevice, localWindowHandle,
									  L"PresentationVerts.cso", L"PresentationColors.cso");
}

RenderManager::~RenderManager()
{
	// Delete heap-allocated data within the ray-marching shader
	rayMarcher->~RayMarcher();
	rayMarcher = nullptr;

	// Delete heap-allocated data within the post-processing shader
	screenPainter->~ScreenPainter();
	screenPainter = nullptr;
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
	this->Display(viewFinderPttr);

	// Present the post-processed scene to the display
	d3D->EndScene();
}

void RenderManager::RenderScene(Camera* mainCamera,
								ID3D11ShaderResourceView* gpuReadableSceneDataView,
								ID3D11UnorderedAccessView* gpuWritableSceneDataView,
								fourByteUnsigned numSceneFigures)
{
	// Pre-fill the scene texture with the forms defined in the given
	// distance functions; also perform post-processing on pixels shaded
	// in the previous render pass (if any)
	// Also pass in a shader-friendly view of the GPU RNG state buffer so we
	// can use the Xorshift random number generator as a noise source for colors/textures,
	// planetary/asteroid terrain, etc.
	ID3D11UnorderedAccessView* gpuRandView = AthruGPU::GPUServiceCentre::AccessGPURandView();
	rayMarcher->Dispatch(d3dContext,
						 mainCamera->GetTranslation(),
						 mainCamera->GetViewMatrix(),
						 gpuReadableSceneDataView,
						 gpuWritableSceneDataView,
						 gpuRandView,
						 numSceneFigures);

	// Apply global illumination to the pixels illuminated during primary
	// ray-marching
	// All the values we're going to need to access were passed to the
	// GPU when we dispatched the ray-marcher, so no need to send
	// anything along this time
	pathTracer->Dispatch(d3dContext);

	// Apply post-processing to the render pass evaluated during the current
	// frame
	// As above, all the values needed for post-processing were already loaded
	// during ray-marching
	postProcessor->Dispatch(d3dContext);
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
