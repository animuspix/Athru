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

	// Construct the display texture
	// Create display texture description
	D3D11_TEXTURE2D_DESC screenTextureDesc;
	screenTextureDesc.Width = GraphicsStuff::DISPLAY_WIDTH;
	screenTextureDesc.Height = GraphicsStuff::DISPLAY_HEIGHT;
	screenTextureDesc.MipLevels = 1;
	screenTextureDesc.ArraySize = 1;
	screenTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	screenTextureDesc.SampleDesc.Count = 1;
	screenTextureDesc.SampleDesc.Quality = 0;
	screenTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	screenTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	screenTextureDesc.CPUAccessFlags = 0;
	screenTextureDesc.MiscFlags = 0;

	// Cache a local reference to the Direct3D rendering device
	ID3D11Device* d3dDevice = d3D->GetDevice();

	// Build texture
	HRESULT result = d3dDevice->CreateTexture2D(&screenTextureDesc, nullptr, &(displayTex.raw));
	assert(SUCCEEDED(result));

	// Extract a read-only shader-friendly resource view from the generated texture
	result = d3dDevice->CreateShaderResourceView(displayTex.raw, nullptr, &(displayTex.asReadOnlyShaderResource));
	assert(SUCCEEDED(result));

	// Extract a writable shader resource view from the generated texture
	result = d3dDevice->CreateUnorderedAccessView(displayTex.raw, nullptr, &(displayTex.asWritableShaderResource));
	assert(SUCCEEDED(result));

	// Construct the shaders used by [this]

	// Cache a local copy of the window handle
	HWND localWindowHandle = AthruUtilities::UtilityServiceCentre::AccessApp()->GetHWND();

	// Construct the path tracer and the image presentation shader
	pathTracer = new PathTracer(L"SceneVis.cso");
	screenPainter = new ScreenPainter(d3dDevice, localWindowHandle,
									  L"PresentationVerts.cso", L"PresentationColors.cso");
}

RenderManager::~RenderManager()
{
	// Delete heap-allocated data within the path-tracer
	pathTracer->~PathTracer();
	pathTracer = nullptr;

	// Delete heap-allocated data within the presentation shader
	screenPainter->~ScreenPainter();
	screenPainter = nullptr;

	// Release DirectX data associated with the display texture
	displayTex.raw->Release();
	displayTex.raw = nullptr;
	displayTex.asReadOnlyShaderResource->Release();
	displayTex.asReadOnlyShaderResource = nullptr;
	displayTex.asWritableShaderResource->Release();
	displayTex.asWritableShaderResource = nullptr;
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
	// Path-trace + post-process the scene
	pathTracer->Dispatch(d3dContext,
						 mainCamera->GetTranslation(),
						 mainCamera->GetViewMatrix(),
						 displayTex.asWritableShaderResource);
}

void RenderManager::Display(ScreenRect* screenRect)
{
	// Pass the rect onto the GPU, then render it with the post-processing shader
	screenRect->PassToGPU(d3dContext);
	screenPainter->Render(d3dContext,
						  displayTex.asReadOnlyShaderResource);
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
