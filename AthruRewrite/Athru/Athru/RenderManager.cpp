// Engine services; the only service used here is the
// Athru custom memory allocator, but it's still best to
// store/access them all in/from the same place
#include "ServiceCentre.h"

// Shaders
#include "Rasterizer.h"
#include "PostProcessor.h"

// Generic objects relevant to [this]
#include "Boxecule.h"
#include "DemoMeshImport.h"
#include "Camera.h"

// The header containing declarations used by [this]
#include "RenderManager.h"

RenderManager::RenderManager(ID3D11DeviceContext* d3dDeviceContext, ID3D11Device* d3dDevice,
							 AVAILABLE_POST_EFFECTS defaultPostEffectA, AVAILABLE_POST_EFFECTS defaultPostEffectB,
							 ID3D11ShaderResourceView* postProcessShaderResource)
{
	// Cache a class-scope reference to the device context
	deviceContext = d3dDeviceContext;

	// Initialise render queue/render queue length storage, object shader,
	// and lighting info

	// Initialise render queue + render queue length storage
	renderQueue = new Boxecule*[MAX_NUM_STORED_BOXECULES];
	renderQueueLength = 0;

	// Initialise the object shader + the post-processing shader

	// Cache a local copy of the window handle
	HWND localWindowHandle = ServiceCentre::AccessApp()->GetHWND();
	rasterizer = new Rasterizer(d3dDevice, localWindowHandle, L"VertPlotter.cso", L"Colorizer.cso");
	postProcessor = new PostProcessor(d3dDevice, localWindowHandle, L"PostVertPlotter.cso", L"PostColorizer.cso",
									  postProcessShaderResource);

	// Initialise lighting data
	visibleLights = new fourByteUnsigned[MAX_POINT_LIGHT_COUNT + MAX_SPOT_LIGHT_COUNT + MAX_DIRECTIONAL_LIGHT_COUNT];
	visibleLightCount = 0;

	// Initialise culling function pointers

	lightingShaderFilterDispatch[0] = &RenderManager::LightDiscard;
	lightingShaderFilterDispatch[1] = &RenderManager::LightCache;

	coreBoxeculeDispatch[0] = &RenderManager::BoxeculeDiscard;
	coreBoxeculeDispatch[1] = &RenderManager::BoxeculeCache;

	// Build temporary imported mesh
	tempExternalMesh = new DemoMeshImport(d3dDevice);

	// Translate the imported mesh so it sits outside of the boxecule cube
	tempExternalMesh->FetchTransformations().pos = _mm_set_ps(1, 0, CHUNK_WIDTH * 2, 0);
}

RenderManager::~RenderManager()
{
	// Delete heap-allocated data within the object shader
	rasterizer->~Rasterizer();
	rasterizer = nullptr;

	// Delete heap-allocated data within the post-processing shader
	postProcessor->~PostProcessor();
	postProcessor = nullptr;

	// Delete visible-light array
	delete visibleLights;
	visibleLights = nullptr;

	// Delete the render queue, then send it to [nullptr]
	delete renderQueue;
	renderQueue = nullptr;

	// Delete the external mesh, then send it to [nullptr]
	delete tempExternalMesh;
	tempExternalMesh = nullptr;

	// Flush any pipeline data associated with [this]
	//ServiceCentre::AccessGraphics()->GetD3D()->GetDeviceContext()->ClearState();
	//ServiceCentre::AccessGraphics()->GetD3D()->GetDeviceContext()->Flush();
}

void RenderManager::RasterizerRender(Material& renderableMaterial, Material& directionalMaterial,
									 Material* pointLightMaterials, Material* spotLightMaterials,
									 DirectX::XMVECTOR& directionalPosition, DirectX::XMVECTOR& directionalRotationQtn,
									 DirectX::XMVECTOR* pointLightPositions, DirectX::XMVECTOR* pointLightRotationQtns,
									 fourByteUnsigned numPointLights,
									 DirectX::XMVECTOR* spotLightPositions, DirectX::XMVECTOR* spotLightRotationQtns,
									 fourByteUnsigned numSpotLights,
									 DirectX::XMMATRIX& worldByModel, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection,
									 ID3D11DeviceContext* deviceContext,
									 fourByteUnsigned numIndicesDrawing)
{
	// Pass the given light position into a shader-friendly [XMFLOAT4]
	DirectX::XMFLOAT4 dirLightPos;
	DirectX::XMStoreFloat4(&dirLightPos, directionalPosition);

	// Apply the light's rotation to the global Z-vector to generate a
	// value representing it's local Z-vector (you could call this it's
	// "forward" vector, since we generally think of "forward" as
	// "towards the viewing horizon" and the viewing horizon is
	// perpendicular to Z in DirectX)
	DirectX::XMVECTOR globalZ = _mm_set_ps(1, 1, 0, 0);
	DirectX::XMVECTOR localZ = DirectX::XMVector3Rotate(globalZ, directionalRotationQtn);

	// Yep, directional light direction is locked at local-Z for simplicity and to avoid
	// storing too many variants of [Luminance]/storing a directional value for point
	// lights that definitely won't use it; this is the part where we take the local-Z
	// vector from above and store it in a shader-friendly [XMFLOAT4]
	DirectX::XMFLOAT4 dirLightDirection;
	DirectX::XMStoreFloat4(&dirLightDirection, localZ);

	float pointLightIntensities[MAX_POINT_LIGHT_COUNT];
	DirectX::XMFLOAT4 pointLightDiffuseColors[MAX_POINT_LIGHT_COUNT];
	DirectX::XMFLOAT4 pointLightLocations[MAX_POINT_LIGHT_COUNT];
	for (fourByteUnsigned i = 0; i < numPointLights; i += 1)
	{
		pointLightIntensities[i] = pointLightMaterials[i].GetLightData().intensity;
		pointLightDiffuseColors[i] = pointLightMaterials[i].GetColorData();
		DirectX::XMStoreFloat4(&(pointLightLocations[i]), pointLightPositions[i]);
	}

	// Cache spot light intensities, diffuse colors, positions, and
	// directions
	float spotLightIntensities[MAX_SPOT_LIGHT_COUNT];
	DirectX::XMFLOAT4 spotLightDiffuseColors[MAX_SPOT_LIGHT_COUNT];
	DirectX::XMFLOAT4 spotLightLocations[MAX_SPOT_LIGHT_COUNT];
	DirectX::XMFLOAT4 spotLightDirections[MAX_SPOT_LIGHT_COUNT];
	DirectX::XMVECTOR globalNegaY = _mm_set_ps(1, 0, -1, 0);
	for (fourByteUnsigned i = 0; i < numSpotLights; i += 1)
	{
		spotLightIntensities[i] = spotLightMaterials[i].GetLightData().intensity;
		spotLightDiffuseColors[i] = spotLightMaterials[i].GetColorData();
		DirectX::XMStoreFloat4(&(spotLightLocations[i]), spotLightPositions[i]);

		// Directional lights always point "forward" (along the local-Z axis) relative to their orientation;
		// similarly, spot light direction is locked at negative-local-Y ("downward") for simplicity and to avoid
		// storing too many variants of [Luminance]/storing a directional value for point lights that
		// definitely won't use it
		DirectX::XMVECTOR localNegaY = DirectX::XMVector3Rotate(globalNegaY, spotLightRotationQtns[i]);
		DirectX::XMStoreFloat4(&(spotLightDirections[i]), localNegaY);
	}

	// Cache the directional light's diffuse color and use it to generate the ambient light for this render
	DirectX::XMFLOAT4 dirDiffuse = directionalMaterial.GetColorData();
	DirectX::XMFLOAT4 dirAmbient = DirectX::XMFLOAT4(dirDiffuse.x * AMBIENT_DIFFUSE_RATIO, dirDiffuse.y * AMBIENT_DIFFUSE_RATIO,
													 dirDiffuse.z * AMBIENT_DIFFUSE_RATIO, 1.0f);

	rasterizer->Render(deviceContext,
					   directionalMaterial.GetLightData().intensity, dirLightDirection,
					   dirDiffuse, dirAmbient, dirLightPos,
					   pointLightIntensities, pointLightDiffuseColors, pointLightLocations,
					   numPointLights,
					   spotLightIntensities, spotLightDiffuseColors, spotLightLocations,
					   spotLightDirections,
					   numSpotLights,
					   worldByModel, view, projection,
					   renderableMaterial.GetTexture().asShaderResource,
					   numIndicesDrawing);
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
		if (lightMaterial.GetLightData().illuminationType == AVAILABLE_ILLUMINATION_TYPES::DIRECTIONAL)
		{
			directionalMaterials[directionalCount] = lightMaterial;
			directionalLightPositions[directionalCount] = lightPosition;
			directionalLightRotationQtns[directionalCount] = lightRotation;
			directionalCount += 1;
		}

		else if (lightMaterial.GetLightData().illuminationType == AVAILABLE_ILLUMINATION_TYPES::POINT)
		{
			pointMaterials[pointCount] = lightMaterial;
			pointLightPositions[pointCount] = lightPosition;
			pointLightRotationQtns[pointCount] = lightRotation;
			pointCount += 1;
		}

		else if (lightMaterial.GetLightData().illuminationType == AVAILABLE_ILLUMINATION_TYPES::SPOT)
		{
			spotMaterials[spotCount] = lightMaterial;
			spotLightPositions[spotCount] = lightPosition;
			spotLightRotationQtns[spotCount] = lightRotation;
			spotCount += 1;
		}
	}

	// Render each boxecule in the render queue (+ the external mesh)
	DirectX::XMVECTOR& cameraPos = mainCamera->GetTranslation();
	if (directionalCount > 0)
	{
		for (fourByteSigned i = 0; i < renderQueueLength; i += 1)
		{
			Boxecule* renderable = renderQueue[i];
			renderable->PassToGPU(deviceContext);
			RasterizerRender(renderable->GetMaterial(), directionalMaterials[0], pointMaterials, spotMaterials,
							 DirectX::XMVector3Transform(directionalLightPositions[0], world), directionalLightRotationQtns[0],
							 pointLightPositions, pointLightRotationQtns, pointCount,
							 spotLightPositions, spotLightRotationQtns, spotCount,
							 world * renderable->GetTransform(), view, projection,
							 deviceContext,
							 BOXECULE_INDEX_COUNT);
		}

		tempExternalMesh->PassToGPU(deviceContext);
		RasterizerRender(tempExternalMesh->GetMaterial(), directionalMaterials[0], pointMaterials, spotMaterials,
						 DirectX::XMVector3Transform(directionalLightPositions[0], world), directionalLightRotationQtns[0],
						 pointLightPositions, pointLightRotationQtns, pointCount,
						 spotLightPositions, spotLightRotationQtns, spotCount,
						 world * tempExternalMesh->GetTransform(), view, projection,
						 deviceContext,
						 tempExternalMesh->GetIndexCount());
	}

	// Nothing left to draw on this pass, so zero the length of the render queue
	renderQueueLength = 0;

	// Also zero the visible light counter
	visibleLightCount = 0;
}

void RenderManager::PostProcess(AthruRect* screenRect,
								DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
{
	// Pass the rect onto the GPU, then render it with the post-processing shader
	screenRect->PassToGPU(deviceContext);
	postProcessor->Render(deviceContext, world * screenRect->GetTransform(), view, projection,
						  true, false, false);
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

fourByteUnsigned RenderManager::SubChunkCache(SubChunk* subChunk, bool withinHome, twoByteUnsigned boxeculeDensity, Camera* mainCamera, fourByteUnsigned unculledCounter)
{
	// Cache a local reference to the boxecules in the current [SubChunk]
	Boxecule** subChunkBoxecules = subChunk->GetStoredBoxecules();

	for (eightByteUnsigned i = 0; i < SUB_CHUNK_VOLUME * ((((twoByteUnsigned)withinHome) * boxeculeDensity) + (1 * !withinHome)); i += 1)
	{
		// Cache a reference to the current boxecule
		Boxecule* currBoxecule = subChunkBoxecules[i];

		// Dispatch boxecules as appropriate and return any changes to the length of the render queue + the number of unculled items
		byteUnsigned dispatchIndex = (byteUnsigned)(currBoxecule->GetMaterial().GetColorData().w != 0);
		fourByteUnsigned resultNumeral = (this->*(this->coreBoxeculeDispatch[dispatchIndex]))(currBoxecule, unculledCounter);
		renderQueueLength += resultNumeral;
		unculledCounter += resultNumeral;
	}

	return unculledCounter;
}

fourByteUnsigned RenderManager::ChunkCache(Chunk* chunk, bool isHome, twoByteUnsigned boxeculeDensity, Camera* mainCamera, fourByteUnsigned unculledCounter)
{
	SubChunk** chunkChildren = chunk->GetSubChunks();

	for (byteUnsigned i = 0; i < SUB_CHUNKS_PER_CHUNK; i += 1)
	{
		unculledCounter = SubChunkCache(chunkChildren[i], isHome, boxeculeDensity, mainCamera, unculledCounter);
	}

	return unculledCounter;
}

void RenderManager::Prepare(Chunk** boxeculeChunks, Camera* mainCamera, twoByteUnsigned boxeculeDensity)
{
	// Implement octree culling here...

	// Copy boxecules from all chunks while performing directional/opacity
	// culling
	byteUnsigned chunkIndex = 0;
	fourByteUnsigned unculledCounter = 0;
	for (eightByteUnsigned i = 0; i < CHUNK_COUNT; i += 1)
	{
		unculledCounter = ChunkCache(boxeculeChunks[chunkIndex], chunkIndex == HOME_CHUNK_INDEX, boxeculeDensity, mainCamera, unculledCounter);
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
