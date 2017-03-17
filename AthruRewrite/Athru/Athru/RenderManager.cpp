#include "ServiceCentre.h"
#include "Rasterizer.h"
#include "Boxecule.h"
#include "Camera.h"
#include "Chunk.h"
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

	// Initialize the function pointers accessed within [Prepare(...)]
	basicBoxeculeDispatch[0] = &RenderManager::DirectionalCullFailed;
	basicBoxeculeDispatch[1] = &RenderManager::DirectionalCullPassed;

	coreBoxeculeDispatch[0] = &RenderManager::BoxeculeDiscard;
	coreBoxeculeDispatch[1] = &RenderManager::BoxeculeCache;

	chunkDispatch[0] = &RenderManager::ChunkDiscard;
	chunkDispatch[1] = &RenderManager::ChunkCache;
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

bool RenderManager::BoxeculeDiscard(Boxecule* boxecule, fourByteUnsigned unculledCounter) { return false; }
bool RenderManager::BoxeculeCache(Boxecule* boxecule, fourByteUnsigned unculledCounter)
{
	renderQueue[unculledCounter] = boxecule;
	return true;
}

bool RenderManager::DirectionalCullFailed(Boxecule* boxecule, Camera* mainCamera, fourByteUnsigned unculledCounter) { return false; }
bool RenderManager::DirectionalCullPassed(Boxecule* boxecule, Camera* mainCamera, fourByteUnsigned unculledCounter)
{
	byteUnsigned dispatchIndex = (byteUnsigned)(mainCamera->IsIntersecting(boxecule) &&
											   (boxecule->GetMaterial().GetColorData()[3] != 0));

	return (this->*(this->coreBoxeculeDispatch[dispatchIndex]))(boxecule, unculledCounter);
}

fourByteUnsigned RenderManager::ChunkDiscard(Chunk* chunk, bool isHome, twoByteUnsigned boxeculeDensity, Camera* mainCamera, fourByteUnsigned unculledCounter) { return unculledCounter; }
fourByteUnsigned RenderManager::ChunkCache(Chunk* chunk, bool isHome, twoByteUnsigned boxeculeDensity, Camera* mainCamera, fourByteUnsigned unculledCounter)
{
	for (eightByteUnsigned j = 0; j < CHUNK_VOLUME * ((((twoByteUnsigned)isHome) * boxeculeDensity) + (1 * !isHome)); j += 1)
	{
		Boxecule* currBoxecule = chunk->GetChunkBoxecules()[j];
		float cameraLocalZ = DirectX::XMVector3Rotate(mainCamera->GetTranslation(), mainCamera->GetRotation()).m128_f32[2];
		float boxeculeGlobalZ = currBoxecule->FetchTransformations().pos.m128_f32[2];

		fourByteUnsigned resultNumeral = (fourByteUnsigned)((this->*(this->basicBoxeculeDispatch[(byteUnsigned)(/*boxeculeGlobalZ > cameraLocalZ*/true)]))(currBoxecule, mainCamera, unculledCounter));
		renderQueueLength += resultNumeral;
		unculledCounter += resultNumeral;
	}

	return unculledCounter;
}

bool RenderManager::ChunkVisible(Chunk* chunk, Camera* mainCamera)
{
	// Generate a view triangle so we can check chunk visibility
	// without expensive frustum testing (chunks exist on a plane anyway,
	// so a view triangle is a good analogue to the view frustum we use
	// with boxecules)
	DirectX::XMVECTOR viewTriVertA = mainCamera->GetTranslation();
	DirectX::XMVECTOR viewTriVertB = DirectX::XMVector3Rotate(_mm_add_ps(viewTriVertA, _mm_set_ps(0, SCREEN_FAR, 0, tan(VERT_FIELD_OF_VIEW_RADS / 2) * SCREEN_FAR)), mainCamera->GetRotation());
	DirectX::XMVECTOR viewTriVertC = DirectX::XMVector3Rotate(_mm_add_ps(viewTriVertA, _mm_set_ps(0, SCREEN_FAR, 0, (tan(VERT_FIELD_OF_VIEW_RADS / 2) * SCREEN_FAR) * -1)), mainCamera->GetRotation());

	// Point/triangle culling algorithm found here:
	// http://www.nerdparadise.com/math/pointinatriangle
	float crossProductMagnitudes[4][3];
	for (byteUnsigned i = 0; i < 4; i += 1)
	{
		DirectX::XMVECTOR* chunkPoints = chunk->GetChunkPoints();
		DirectX::XMVECTOR vectorDiffA = _mm_sub_ps(chunkPoints[i], viewTriVertA);
		DirectX::XMVECTOR vectorDiffB = _mm_sub_ps(viewTriVertB, viewTriVertA);
		DirectX::XMVECTOR vectorDiffC = _mm_sub_ps(chunkPoints[i], viewTriVertB);

		DirectX::XMVECTOR vectorDiffD = _mm_sub_ps(viewTriVertC, viewTriVertB);
		DirectX::XMVECTOR vectorDiffE = _mm_sub_ps(chunkPoints[i], viewTriVertC);
		DirectX::XMVECTOR vectorDiffF = _mm_sub_ps(viewTriVertA, viewTriVertC);

		crossProductMagnitudes[i][0] = _mm_cvtss_f32(DirectX::XMVector3Length(DirectX::XMVector3Cross(vectorDiffA, vectorDiffB)));
		crossProductMagnitudes[i][1] = _mm_cvtss_f32(DirectX::XMVector3Length(DirectX::XMVector3Cross(vectorDiffC, vectorDiffD)));
		crossProductMagnitudes[i][2] = _mm_cvtss_f32(DirectX::XMVector3Length(DirectX::XMVector3Cross(vectorDiffE, vectorDiffF)));
	}

	return std::signbit(crossProductMagnitudes[0][0]) == std::signbit(crossProductMagnitudes[1][0]) == std::signbit(crossProductMagnitudes[2][0]) == std::signbit(crossProductMagnitudes[3][0]) ||
		   std::signbit(crossProductMagnitudes[0][1]) == std::signbit(crossProductMagnitudes[1][1]) == std::signbit(crossProductMagnitudes[2][1]) == std::signbit(crossProductMagnitudes[3][1]) ||
		   std::signbit(crossProductMagnitudes[0][2]) == std::signbit(crossProductMagnitudes[1][2]) == std::signbit(crossProductMagnitudes[2][2]) == std::signbit(crossProductMagnitudes[3][2]);
}

void RenderManager::Prepare(Chunk** boxeculeChunks, Camera* mainCamera, twoByteUnsigned boxeculeDensity)
{
	// Chunk culling here
	// Failing chunk culling means skipping boxecule culling/preparation for the
	// failed chunk
	bool chunkVisibilities[9] = { ChunkVisible(boxeculeChunks[0], mainCamera),
								  ChunkVisible(boxeculeChunks[1], mainCamera),
								  ChunkVisible(boxeculeChunks[2], mainCamera),
								  ChunkVisible(boxeculeChunks[3], mainCamera),
								  ChunkVisible(boxeculeChunks[4], mainCamera),
								  ChunkVisible(boxeculeChunks[5], mainCamera),
								  ChunkVisible(boxeculeChunks[6], mainCamera),
								  ChunkVisible(boxeculeChunks[7], mainCamera),
								  ChunkVisible(boxeculeChunks[8], mainCamera) };

	// Copy boxecules from non-home chunks while performing directional/frustum/opacity
	// culling
	byteUnsigned chunkIndex = 0;
	fourByteUnsigned unculledCounter = 0;
	for (eightByteUnsigned i = 0; i < (9 * CHUNK_VOLUME); i += CHUNK_VOLUME)
	{
		unculledCounter = (this->*(this->chunkDispatch[(byteUnsigned)/*(chunkVisibilities[chunkIndex])*/true]))
								  (boxeculeChunks[chunkIndex], chunkIndex == HOME_CHUNK_INDEX, boxeculeDensity, mainCamera, unculledCounter);
		chunkIndex += 1;
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
