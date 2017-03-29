// Engine services; the only service used here is the
// Athru custom memory allocator, but it's still best to
// store/access them all in/from the same place
#include "ServiceCentre.h"

// Shaders (object first, then lighting, then post-effects)
#include "Rasterizer.h"
#include "TexturedRasterizer.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "Bloom.h"
#include "DepthOfField.h"

// Generic objects relevant to [this]
#include "Boxecule.h"
#include "Camera.h"

// The header containing declarations used by [this]
#include "RenderManager.h"

RenderManager::RenderManager(ID3D11DeviceContext* d3dDeviceContext, ID3D11Device* d3dDevice,
							 AVAILABLE_POST_EFFECTS defaultPostEffectA, AVAILABLE_POST_EFFECTS defaultPostEffectB, AVAILABLE_POST_EFFECTS defaultPostEffectC)
{
	// Cache a class-scope reference to the device context
	deviceContext = d3dDeviceContext;

	// Initialise render queue + shaders

	renderQueue = new Boxecule*[MAX_NUM_STORED_BOXECULES];
	renderQueueLength = 0;

	// Memoize the window handle (required for shader initialisation)
	HWND windowHandle = ServiceCentre::AccessApp()->GetHWND();

	// Initialise the object shader
	rasterizer = new Rasterizer(d3dDevice, windowHandle, L"VertPlotter.cso", L"Colorizer.cso");

	// Initialise lighting data
	visibleLights = new fourByteUnsigned[MAX_POINT_LIGHT_COUNT + MAX_SPOT_LIGHT_COUNT + MAX_DIRECTIONAL_LIGHT_COUNT];
	visibleLightCount = 0;

	// Initialise culling function pointers

	lightingShaderFilterDispatch[0] = &RenderManager::LightDiscard;
	lightingShaderFilterDispatch[1] = &RenderManager::LightCache;

	basicBoxeculeDispatch[0] = &RenderManager::DirectionalCullFailed;
	basicBoxeculeDispatch[1] = &RenderManager::DirectionalCullPassed;

	coreBoxeculeDispatch[0] = &RenderManager::BoxeculeDiscard;
	coreBoxeculeDispatch[1] = &RenderManager::BoxeculeCache;

	chunkDispatch[0] = &RenderManager::ChunkDiscard;
	chunkDispatch[1] = &RenderManager::ChunkCache;

	subChunkDispatch[0] = &RenderManager::SubChunkDiscard;
	subChunkDispatch[1] = &RenderManager::SubChunkCache;
}

RenderManager::~RenderManager()
{
	// Delete heap-allocated data within the object shader
	rasterizer->~Rasterizer();

	// Delete heap-allocated data within each shadow shader
	//for (byteUnsigned i = 0; i < shadowShaderCount; i += 1)
	//{
	//	availableShadowShaders[i]->~Shader();
	//}

	// Delete heap-allocated data within each post-effect shader
	//bloom->~Bloom();
	//depthOfField->~DepthOfField();

	// Delete the render queue, then send it to [nullptr]
	delete renderQueue;
	renderQueue = nullptr;
}

void RenderManager::RasterizerRender(Material& renderableMaterial, Material& directionalMaterial,
									 Material* pointLightMaterials, Material* spotLightMaterials,
									 DirectX::XMVECTOR& directionalPosition, DirectX::XMVECTOR& directionalRotationQtn,
									 DirectX::XMVECTOR* pointLightPositions, DirectX::XMVECTOR* pointLightRotationQtns,
									 DirectX::XMVECTOR* spotLightPositions, DirectX::XMVECTOR* spotLightRotationQtns,
									 DirectX::XMMATRIX& worldByModel, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection,
									 ID3D11DeviceContext* deviceContext,
									 fourByteUnsigned numIndicesDrawing)
{
	// Pass the given light position into a shader-friendly [XMFLOAT4]
	DirectX::XMFLOAT4 lightPos;
	DirectX::XMStoreFloat4(&lightPos, directionalPosition);

	// Apply the light's rotation to the global Z-vector to generate a
	// value representing it's local Z-vector (you could call this it's
	// "forward" vector, since we generally think of "forward" as
	// "towards the viewing horizon" and the viewing horizon is
	// perpendicular to Z in DirectX)
	DirectX::XMVECTOR globalZ = _mm_set_ps(1, 1, 0, 0);
	DirectX::XMVECTOR localZ = DirectX::XMVector3Rotate(globalZ, directionalRotationQtn);

	// Yep, light direction is locked at local-Z for simplicity and to avoid storing
	// too many variants of [Luminance]/storing a directional value for point lights
	// that definitely won't use it; this is the part where we take the local-Z
	// vector from above and store it in a shader-friendly [XMFLOAT4]
	DirectX::XMFLOAT4 lightDirection;
	DirectX::XMStoreFloat4(&lightDirection, localZ);

	// No point or spot light processing for now; get the single-shader system working
	// with a directional light first, /then/ upgrade it :)

	// Cache the directional light's diffuse color and use it to generate the ambient light for this render
	DirectX::XMFLOAT4 diffuse = directionalMaterial.GetColorData();
	DirectX::XMFLOAT4 ambient = DirectX::XMFLOAT4(diffuse.x * AMBIENT_DIFFUSE_RATIO, diffuse.y * AMBIENT_DIFFUSE_RATIO,
												  diffuse.z * AMBIENT_DIFFUSE_RATIO, 1.0f);

	rasterizer->Render(deviceContext,
					   DirectX::XMFLOAT4(localIntensity, localIntensity, localIntensity, localIntensity),
					   lightDirection, diffuse, ambient, lightPos,
					   worldByModel, view, projection,
					   renderableMaterial.GetTexture().asShaderResource,
					   numIndicesDrawing);
}

void RenderManager::ProceduralShadowMapperRenderer(Material& renderableMaterial,
												   ID3D11DeviceContext* deviceContext,
												   DirectX::XMMATRIX& worldByModel, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection,
												   fourByteUnsigned numIndicesDrawing)
{
	// Nothing for now...
}

void RenderManager::BloomRender(ID3D11ShaderResourceView* rasterBuffer,
								ID3D11ShaderResourceView* lightBuffer,
								ID3D11ShaderResourceView* postBuffer,
								ID3D11DeviceContext* deviceContext,
							    DirectX::XMMATRIX& worldByModel, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection,
							    fourByteUnsigned numIndicesDrawing)
{
	// Nothing for now...
}

void RenderManager::DepthOfFieldRender(ID3D11ShaderResourceView* rasterBuffer,
									   ID3D11ShaderResourceView* lightBuffer,
									   ID3D11ShaderResourceView* postBuffer,
									   ID3D11DeviceContext* deviceContext,
									   DirectX::XMMATRIX& worldByModel, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection,
									   fourByteUnsigned numIndicesDrawing)
{
	// Nothing for now...
}

void RenderManager::Render(Camera* mainCamera,
						   DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
{
	// Get each item from the render queue
	// Pass it to the GPU
	// Render it with the Rasterizer (basically an all-in-one shader)

	// Cache arrays of the materials associated with every visible light
	// Also cache arrays of light positions and rotations

	Material directionalMaterials[MAX_DIRECTIONAL_LIGHT_COUNT];
	DirectX::XMVECTOR directionalLightPositions[MAX_DIRECTIONAL_LIGHT_COUNT];
	DirectX::XMVECTOR directionalLightRotationQtns[MAX_DIRECTIONAL_LIGHT_COUNT];
	byteUnsigned directionalCount = 0;

	Material pointMaterials[MAX_POINT_LIGHT_COUNT];
	DirectX::XMVECTOR pointLightPositions[MAX_POINT_LIGHT_COUNT];
	DirectX::XMVECTOR pointLightRotationQtns[MAX_POINT_LIGHT_COUNT];
	byteUnsigned pointCount = 0;

	Material spotMaterials[MAX_SPOT_LIGHT_COUNT];
	DirectX::XMVECTOR spotLightPositions[MAX_SPOT_LIGHT_COUNT];
	DirectX::XMVECTOR spotLightRotationQtns[MAX_SPOT_LIGHT_COUNT];
	byteUnsigned spotCount = 0;

	for (fourByteUnsigned j = 0; j < visibleLightCount; j += 1)
	{
		Material lightMaterial = renderQueue[visibleLights[j]]->GetMaterial();
		DirectX::XMVECTOR& lightPosition = renderQueue[visibleLights[j]]->FetchTransformations().pos;
		DirectX::XMVECTOR& lightRotation = renderQueue[visibleLights[j]]->FetchTransformations().rotationQuaternion;
		if (lightMaterial.GetLightData().illuminationType == AVAILABLE_LIGHTING_SHADERS::DIRECTIONAL_LIGHT)
		{
			directionalMaterials[directionalCount] = lightMaterial;
			directionalLightPositions[directionalCount] = lightPosition;
			directionalLightRotationQtns[directionalCount] = lightRotation;
			directionalCount += 1;
		}

		else if (lightMaterial.GetLightData().illuminationType == AVAILABLE_LIGHTING_SHADERS::POINT_LIGHT)
		{
			pointMaterials[pointCount] = lightMaterial;
			pointLightPositions[pointCount] = lightPosition;
			pointLightRotationQtns[pointCount] = lightRotation;
			pointCount += 1;
		}

		else if (lightMaterial.GetLightData().illuminationType == AVAILABLE_LIGHTING_SHADERS::SPOT_LIGHT)
		{
			spotMaterials[spotCount] = lightMaterial;
			spotLightPositions[spotCount] = lightPosition;
			spotLightRotationQtns[spotCount] = lightRotation;
			spotCount += 1;
		}
	}

	// Render each boxecule in the render queue
	DirectX::XMVECTOR& cameraPos = mainCamera->GetTranslation();
	for (fourByteSigned i = 0; i < renderQueueLength; i += 1)
	{
		Boxecule* renderable = renderQueue[i];
		renderable->PassToGPU(deviceContext);
		RasterizerRender(renderable->GetMaterial(), directionalMaterials[0], pointMaterials, spotMaterials,
						 DirectX::XMVector3Transform(directionalLightPositions[0], world), directionalLightRotationQtns[0],
						 pointLightPositions, pointLightRotationQtns,
						 spotLightPositions, spotLightRotationQtns,
						 world * renderable->GetTransform(), view, projection,
						 deviceContext,
						 BOXECULE_INDEX_COUNT);
	}

	// Nothing left to draw on this pass, so zero the length of the render queue
	renderQueueLength = 0;

	// Also zero the visible light counter
	visibleLightCount = 0;
}

void RenderManager::LightDiscard(fourByteUnsigned boxeculeIndex) {}
void RenderManager::LightCache(fourByteUnsigned boxeculeIndex)
{
	visibleLights[visibleLightCount] = boxeculeIndex;
	visibleLightCount += 1;
}

bool RenderManager::BoxeculeDiscard(Boxecule* boxecule, fourByteUnsigned unculledCounter) { return false; }
bool RenderManager::BoxeculeCache(Boxecule* boxecule, fourByteUnsigned unculledCounter)
{
	renderQueue[unculledCounter] = boxecule;
	byteUnsigned dispatchIndex = (byteUnsigned)(boxecule->GetMaterial().GetLightData().intensity != 0);
	(this->*(this->lightingShaderFilterDispatch[dispatchIndex]))(unculledCounter); // Only unculled boxecules will enter the render-queue, so treating [unculledCounter] like an index should be fine
	return true;
}

bool RenderManager::DirectionalCullFailed(Boxecule* boxecule, Camera* mainCamera, fourByteUnsigned unculledCounter) { return false; }
bool RenderManager::DirectionalCullPassed(Boxecule* boxecule, Camera* mainCamera, fourByteUnsigned unculledCounter)
{
	byteUnsigned dispatchIndex = (byteUnsigned)(boxecule->GetMaterial().GetColorData().w != 0);
	return (this->*(this->coreBoxeculeDispatch[dispatchIndex]))(boxecule, unculledCounter);
}

fourByteUnsigned RenderManager::SubChunkDiscard(SubChunk* subChunk, bool withinHome, twoByteUnsigned boxeculeDensity, Camera* mainCamera, fourByteUnsigned unculledCounter) { return unculledCounter; }
fourByteUnsigned RenderManager::SubChunkCache(SubChunk* subChunk, bool withinHome, twoByteUnsigned boxeculeDensity, Camera* mainCamera, fourByteUnsigned unculledCounter)
{
	Boxecule** subChunkBoxecules = subChunk->GetStoredBoxecules();

	for (eightByteUnsigned i = 0; i < SUB_CHUNK_VOLUME * ((((twoByteUnsigned)withinHome) * boxeculeDensity) + (1 * !withinHome)); i += 1)
	{
		Boxecule* currBoxecule = subChunkBoxecules[i];
		float cameraLocalZ = DirectX::XMVector3Rotate(mainCamera->GetTranslation(), mainCamera->GetRotationQuaternion()).m128_f32[2];
		float boxeculeGlobalZ = currBoxecule->FetchTransformations().pos.m128_f32[2];

		fourByteUnsigned resultNumeral = (fourByteUnsigned)((this->*(this->basicBoxeculeDispatch[(byteUnsigned)(boxeculeGlobalZ > cameraLocalZ)]))(currBoxecule, mainCamera, unculledCounter));
		renderQueueLength += resultNumeral;
		unculledCounter += resultNumeral;
	}

	return unculledCounter;
}

fourByteUnsigned RenderManager::ChunkDiscard(Chunk* chunk, bool isHome, twoByteUnsigned boxeculeDensity, Camera* mainCamera, fourByteUnsigned unculledCounter) { return unculledCounter; }
fourByteUnsigned RenderManager::ChunkCache(Chunk* chunk, bool isHome, twoByteUnsigned boxeculeDensity, Camera* mainCamera, fourByteUnsigned unculledCounter)
{
	SubChunk** chunkChildren = chunk->GetSubChunks();

	for (byteUnsigned i = 0; i < SUB_CHUNKS_PER_CHUNK; i += 1)
	{
		unculledCounter = (this->*(this->subChunkDispatch[(byteUnsigned)(chunkChildren[i]->GetVisibility(mainCamera))]))(chunkChildren[i], isHome, boxeculeDensity, mainCamera, unculledCounter);
	}

	return unculledCounter;
}

void RenderManager::Prepare(Chunk** boxeculeChunks, Camera* mainCamera, twoByteUnsigned boxeculeDensity)
{
	// Chunk culling here
	// Failing chunk culling means skipping boxecule culling/preparation for the
	// failed chunk
	bool chunkVisibilities[CHUNK_COUNT] = { boxeculeChunks[0]->GetVisibility(mainCamera),
											boxeculeChunks[1]->GetVisibility(mainCamera),
											boxeculeChunks[2]->GetVisibility(mainCamera),
											boxeculeChunks[3]->GetVisibility(mainCamera),
											boxeculeChunks[4]->GetVisibility(mainCamera),
											boxeculeChunks[5]->GetVisibility(mainCamera),
											boxeculeChunks[6]->GetVisibility(mainCamera),
											boxeculeChunks[7]->GetVisibility(mainCamera),
											boxeculeChunks[8]->GetVisibility(mainCamera) };

	// Copy boxecules from non-home chunks while performing directional/frustum/opacity
	// culling
	byteUnsigned chunkIndex = 0;
	fourByteUnsigned unculledCounter = 0;
	for (eightByteUnsigned i = 0; i < CHUNK_COUNT; i += 1)
	{
		unculledCounter = (this->*(this->chunkDispatch[(byteUnsigned)(chunkVisibilities[chunkIndex])]))
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
