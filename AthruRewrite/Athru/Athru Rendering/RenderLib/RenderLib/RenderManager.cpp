// Athru utility services
#include "UtilityServiceCentre.h"

// Athru rendering services
#include "RenderServiceCentre.h"

// The Direct3D handler class
#include "Direct3D.h"

// Shaders
#include "Rasterizer.h"
#include "PostProcessor.h"

// Generic objects relevant to [this]
#include "Boxecule.h"
#include "Camera.h"

// The header containing declarations used by [this]
#include "RenderManager.h"

RenderManager::RenderManager(ID3D11ShaderResourceView* postProcessShaderResource)
{
	// Cache a local copy of the window handle
	HWND localWindowHandle = AthruUtilities::UtilityServiceCentre::AccessApp()->GetHWND();

	// Cache a class-scope reference to the Direct3D handler class
	d3D = AthruRendering::RenderServiceCentre::AccessD3D();

	// Cache a local reference to the Direct3D rendering device
	ID3D11Device* d3dDevice = d3D->GetDevice();

	// Initialise the object shader + the post-processing shader
	rasterizer = new Rasterizer(d3dDevice, localWindowHandle, L"VertPlotter.cso", L"Colorizer.cso");
	postProcessor = new PostProcessor(d3dDevice, localWindowHandle, L"PostVertPlotter.cso", L"PostColorizer.cso",
									  postProcessShaderResource);
}

RenderManager::~RenderManager()
{
	// Delete heap-allocated data within the object shader
	rasterizer->~Rasterizer();
	rasterizer = nullptr;

	// Delete heap-allocated data within the post-processing shader
	postProcessor->~PostProcessor();
	postProcessor = nullptr;
}

void RenderManager::Render(Camera* mainCamera)
{
	// Cache a local reference to the camera's viewfinder
	// (screen rect)
	ScreenRect* viewFinderPttr = mainCamera->GetViewFinder();

	// Prepare DirectX for scene rendering
	d3D->BeginScene(viewFinderPttr->GetRenderTarget());

	// Compute scene data for the current main-camera position
	//this->PreProcess();

	// Render scene to the screen texture
	//RenderScene();

	// Prepare DirectX for post-processing
	d3D->BeginPost();

	// Post-process
	this->PostProcess(viewFinderPttr, d3D->GetWorldMatrix(), mainCamera->GetViewMatrix(), d3D->GetPerspProjector());

	// Present the post-processed scene to the display
	d3D->EndScene();
}

void RenderManager::PostProcess(ScreenRect* screenRect,
								DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
{
	// Pass the rect onto the GPU, then render it with the post-processing shader
	ID3D11DeviceContext* context = d3D->GetDeviceContext();
	screenRect->PassToGPU(context);
	postProcessor->Render(context, world, view, projection,
						  true, false, false);
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
