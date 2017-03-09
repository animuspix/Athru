#include "ServiceCentre.h"
#include "Rasterizer.h"
#include "Boxecule.h"
#include "SceneManager.h"
#include "RenderManager.h"

RenderManager::RenderManager(ID3D11DeviceContext* d3dDeviceContext, ID3D11Device* d3dDevice)
{
	deviceContext = d3dDeviceContext;
	renderQueueLength = 0;

	// More reasons to fix the StackAllocator here...
	renderQueue = new Boxecule*[MAX_NUM_STORED_BOXECULES];

	// Render-time optimisation can't be significantly improved without
	// creating a many-to-one relationship between vertices and shaders;
	// however, I don't know if it's possible to pass multiple objects
	// onto the GPU, and if so, I don't know how I'd go about it
	// I think I'll stay with the current algorithm until Tuesday, then
	// ask Adam about parallelisation
	// I'd like to match shaders to the largest possible number of
	// objects if possible, since that could drastically reduce the
	// number of times especially common shader programs
	// (such as Rasterizer) are processed
	availableShaders = new Shader*;
	availableShaders[(byteUnsigned)AVAILABLE_SHADERS::RASTERIZER] = new Rasterizer(d3dDevice, ServiceCentre::AccessApp()->GetHWND(), L"VertPlotter.cso", L"Colorizer.cso");
}

RenderManager::~RenderManager()
{
	delete availableShaders;
	availableShaders = nullptr;
}

void RenderManager::Render(DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
{
	// Get each item from the render queue
	// Pass it to the GPU
	// Render the relevant shaders

	// No deferred rendering, so display everything with forward rendering for now
	for (fourByteSigned i = 0; i < renderQueueLength; i += 1)
	{
		Boxecule* renderable = renderQueue[i];
		AVAILABLE_SHADERS* shaders = renderable->GetMaterial().GetShaderSet();
		renderable->PassToGPU(deviceContext);

		byteUnsigned j = 0;
		while (shaders[j] != AVAILABLE_SHADERS::NULL_SHADER)
		{
			(*availableShaders)[(byteUnsigned)shaders[j]].Render(deviceContext, world * renderable->GetTransform(), view, projection);
			j += 1;
		}
	}

	// Nothing left to draw on this pass, so zero the length of the render queue
	renderQueueLength = 0;
}

void RenderManager::Prepare(Boxecule** boxeculeSet, Camera* mainCamera)
{
	for (fourByteUnsigned i = 0; i < ServiceCentre::AccessSceneManager()->CurrBoxeculeCount(); i += 1)
	{
		// Frustum culling here...
		//if ([frustum collision check succeeds] &&
		//    (boxeculeSet[i]->GetMaterial().GetColorData()[3] != 0))
		//{
		//	renderQueue[i] = boxeculeSet[i];
		//	renderQueueLength += 1;
		//}
	}
}

// Push constructions for this class through Athru's custom allocator
void* RenderManager::operator new(size_t size)
{
	StackAllocator* allocator = ServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<RenderManager>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void RenderManager::operator delete(void* target)
{
	return;
}
