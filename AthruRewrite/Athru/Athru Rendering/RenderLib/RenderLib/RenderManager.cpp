// Engine services; the only service used here is the
// Athru custom memory allocator, but it's still best to
// store/access them all in/from the same place
#include "UtilityServiceCentre.h"

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

RenderManager::RenderManager(ID3D11DeviceContext* d3dDeviceContext, ID3D11Device* d3dDevice,
							 AVAILABLE_POST_EFFECTS defaultPostEffectA, AVAILABLE_POST_EFFECTS defaultPostEffectB,
							 ID3D11ShaderResourceView* postProcessShaderResource)
{
	// Cache a class-scope reference to the given device
	device = d3dDevice;

	// Cache a class-scope reference to the given device context
	deviceContext = d3dDeviceContext;

	// Initialise the object shader + the post-processing shader

	// Cache a local copy of the window handle
	HWND localWindowHandle = AthruUtilities::UtilityServiceCentre::AccessApp()->GetHWND();
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

void RenderManager::Render(Camera* mainCamera,
						   DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
{
	// Sample terrain
	// Compute lighting
	// Render sampled terrain
	// Post-process
}

void RenderManager::PostProcess(AthruRect* screenRect,
								DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
{
	// Pass the rect onto the GPU, then render it with the post-processing shader
	screenRect->PassToGPU(deviceContext);
	postProcessor->Render(deviceContext, world * screenRect->GetTransform(), view, projection,
						  true, false, false);
}

ID3D11Device * RenderManager::GetD3DDevice()
{
	return device;
}

ID3D11DeviceContext * RenderManager::GetD3DDeviceContext()
{
	return deviceContext;
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
