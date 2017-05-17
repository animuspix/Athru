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
#include "VoxelGrid.h"
#include "Camera.h"

// The header containing declarations used by [this]
#include "RenderManager.h"

RenderManager::RenderManager(ID3D11ShaderResourceView* postProcessShaderResource)
{
	// Cache a class-scope reference to the Direct3D handler class
	d3D = AthruRendering::RenderServiceCentre::AccessD3D();

	// Cache a class-scope reference to the Direct3D rendering context
	d3dContext = d3D->GetDeviceContext();

	// Cache a local reference to the Direct3D rendering device
	ID3D11Device* d3dDevice = d3D->GetDevice();

	// Cache a local copy of the window handle
	HWND localWindowHandle = AthruUtilities::UtilityServiceCentre::AccessApp()->GetHWND();

	// Construct the voxel grid
	voxGrid = new VoxelGrid(d3dDevice);

	// Construct the object shader + the post-processing shader
	rasterizer = new Rasterizer(d3dDevice, localWindowHandle, L"VertPlotter.cso", L"Colorizer.cso");
	postProcessor = new PostProcessor(d3dDevice, localWindowHandle, L"PostVertPlotter.cso", L"PostColorizer.cso",
									  postProcessShaderResource);

	// Cache class-scope references to scene textures
	sceneColorTexture = AthruRendering::RenderServiceCentre::AccessTextureManager()->GetVolumeTexture(AVAILABLE_VOLUME_TEXTURES::SCENE_COLOR_TEXTURE);
	sceneNormalTexture = AthruRendering::RenderServiceCentre::AccessTextureManager()->GetVolumeTexture(AVAILABLE_VOLUME_TEXTURES::SCENE_COLOR_TEXTURE);
	scenePBRTexture = AthruRendering::RenderServiceCentre::AccessTextureManager()->GetVolumeTexture(AVAILABLE_VOLUME_TEXTURES::SCENE_COLOR_TEXTURE);
	sceneEmissivityTexture = AthruRendering::RenderServiceCentre::AccessTextureManager()->GetVolumeTexture(AVAILABLE_VOLUME_TEXTURES::SCENE_COLOR_TEXTURE);

	// Cache class-scope references to effect masks
	blurMask = AthruRendering::RenderServiceCentre::AccessTextureManager()->GetDisplayTexture(AVAILABLE_DISPLAY_TEXTURES::BLUR_MASK);
	brightenMask = AthruRendering::RenderServiceCentre::AccessTextureManager()->GetDisplayTexture(AVAILABLE_DISPLAY_TEXTURES::BRIGHTEN_MASK);
	drugsMask = AthruRendering::RenderServiceCentre::AccessTextureManager()->GetDisplayTexture(AVAILABLE_DISPLAY_TEXTURES::DRUGS_MASK);
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

	// Cache temporary copies of the world, view, and perspective
	// projection matrices
	DirectX::XMMATRIX world = d3D->GetWorldMatrix();
	DirectX::XMMATRIX view = mainCamera->GetViewMatrix();
	DirectX::XMMATRIX perspProjection = d3D->GetPerspProjector();

	// Compute scene data for the current main-camera position
	this->PreProcess();

	// Render scene to the screen texture
	this->RenderScene(world, view, perspProjection);

	// Prepare DirectX for post-processing
	d3D->BeginPost();

	// Post-process
	this->PostProcess(viewFinderPttr, world, view, perspProjection);

	// Present the post-processed scene to the display
	d3D->EndScene();
}

void RenderManager::PreProcess()
{

}

void RenderManager::RenderScene(DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
{
	// Pass the voxel grid onto the GPU, then render it with the object shader
	voxGrid->PassToGPU(d3dContext);
	rasterizer->Render(d3dContext, world, view, projection, sceneColorTexture.asRenderedShaderResource);
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
