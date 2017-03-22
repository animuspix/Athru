// Engine services; the only service used here is the
// Athru custom memory allocator, but it's still best to
// store/access them all in/from the same place
#include "ServiceCentre.h"

// Shaders (object first, then lighting, then post-effects)
#include "Rasterizer.h"
#include "TexturedRasterizer.h"
#include "CookTorrancePBR.h"
#include "Bloom.h"
#include "DepthOfField.h"

// Generic objects relevant to [this]
#include "Boxecule.h"
#include "Camera.h"

// The header containing declarations used by [this]
#include "RenderManager.h"

RenderManager::RenderManager(ID3D11DeviceContext* d3dDeviceContext, ID3D11Device* d3dDevice, D3D11_VIEWPORT& viewport,
							 AVAILABLE_POST_EFFECTS defaultPostEffectA, AVAILABLE_POST_EFFECTS defaultPostEffectB, AVAILABLE_POST_EFFECTS defaultPostEffectC,
							 AVAILABLE_LIGHTING_SHADERS defaultLightingShader) :
							 deviceContext(d3dDeviceContext), viewportRef(viewport)
{
	// Initialise core rendering objects/properties

	// Long integer used to store success/failure for different DirectX operations
	HRESULT result;

	// Setup render-buffer (raw texture view) description
	D3D11_TEXTURE2D_DESC renderBufferDesc;
	ZeroMemory(&renderBufferDesc, sizeof(renderBufferDesc));
	renderBufferDesc.Width = DISPLAY_WIDTH;
	renderBufferDesc.Height = DISPLAY_HEIGHT;
	renderBufferDesc.MipLevels = 1;
	renderBufferDesc.ArraySize = 1;
	renderBufferDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	renderBufferDesc.SampleDesc.Count = 1;
	renderBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	renderBufferDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	renderBufferDesc.CPUAccessFlags = 0;
	renderBufferDesc.MiscFlags = 0;

	// Create render-buffers
	result = d3dDevice->CreateTexture2D(&renderBufferDesc, NULL, &renderBufferTextures[(byteUnsigned)DEFERRED_BUFFER_TYPES::RASTER_BUFFER]);
	assert(SUCCEEDED(result));

	result = d3dDevice->CreateTexture2D(&renderBufferDesc, NULL, &renderBufferTextures[(byteUnsigned)DEFERRED_BUFFER_TYPES::LIGHT_BUFFER]);
	assert(SUCCEEDED(result));

	result = d3dDevice->CreateTexture2D(&renderBufferDesc, NULL, &renderBufferTextures[(byteUnsigned)DEFERRED_BUFFER_TYPES::POST_BUFFER]);
	assert(SUCCEEDED(result));

	// Setup render-buffer (target view) description
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	ZeroMemory(&renderTargetViewDesc, sizeof(renderTargetViewDesc));
	renderTargetViewDesc.Format = renderBufferDesc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	// Create render-buffer targets
	result = d3dDevice->CreateRenderTargetView(renderBufferTextures[0], &renderTargetViewDesc, &renderBufferTargets[(byteUnsigned)DEFERRED_BUFFER_TYPES::RASTER_BUFFER]);
	assert(SUCCEEDED(result));

	result = d3dDevice->CreateRenderTargetView(renderBufferTextures[1], &renderTargetViewDesc, &renderBufferTargets[(byteUnsigned)DEFERRED_BUFFER_TYPES::LIGHT_BUFFER]);
	assert(SUCCEEDED(result));

	result = d3dDevice->CreateRenderTargetView(renderBufferTextures[2], &renderTargetViewDesc, &renderBufferTargets[(byteUnsigned)DEFERRED_BUFFER_TYPES::POST_BUFFER]);
	assert(SUCCEEDED(result));

	// Setup render-buffer (shader resource view) description
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	ZeroMemory(&shaderResourceViewDesc, sizeof(shaderResourceViewDesc));
	shaderResourceViewDesc.Format = renderBufferDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	// Create render-buffer shader resources
	result = d3dDevice->CreateShaderResourceView(renderBufferTextures[0], &shaderResourceViewDesc, &renderBufferShaderResources[(byteUnsigned)DEFERRED_BUFFER_TYPES::RASTER_BUFFER]);
	assert(SUCCEEDED(result));

	result = d3dDevice->CreateShaderResourceView(renderBufferTextures[1], &shaderResourceViewDesc, &renderBufferShaderResources[(byteUnsigned)DEFERRED_BUFFER_TYPES::LIGHT_BUFFER]);
	assert(SUCCEEDED(result));

	result = d3dDevice->CreateShaderResourceView(renderBufferTextures[2], &shaderResourceViewDesc, &renderBufferShaderResources[(byteUnsigned)DEFERRED_BUFFER_TYPES::POST_BUFFER]);
	assert(SUCCEEDED(result));

	// Setup the description for the local depth-stencil buffer (raw texture view)
	D3D11_TEXTURE2D_DESC depthStencilBufferDesc;
	ZeroMemory(&depthStencilBufferDesc, sizeof(depthStencilBufferDesc));
	depthStencilBufferDesc.Width = DISPLAY_WIDTH;
	depthStencilBufferDesc.Height = DISPLAY_WIDTH;
	depthStencilBufferDesc.MipLevels = 1;
	depthStencilBufferDesc.ArraySize = 1;
	depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilBufferDesc.SampleDesc.Count = 1;
	depthStencilBufferDesc.SampleDesc.Quality = 0;
	depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilBufferDesc.CPUAccessFlags = 0;
	depthStencilBufferDesc.MiscFlags = 0;

	// Create the local depth-stencil buffer
	result = d3dDevice->CreateTexture2D(&depthStencilBufferDesc, NULL, &localDepthStencilTexture);
	assert(SUCCEEDED(result));

	// Setup the description for the local depth-stencil buffer (depth-stencil view)
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	// Create the local depth stencil
	result = d3dDevice->CreateDepthStencilView(localDepthStencilTexture, &depthStencilViewDesc, &localDepthStencil);
	assert(SUCCEEDED(result));

	// The device context + viewport references were cached in the member initialization list,
	// so no need to do that here

	// Initialise render queue + shaders

	renderQueue = new Boxecule*[MAX_NUM_STORED_BOXECULES];
	renderQueueLength = 0;

	// Memoize the window handle (required for shader initialisation)
	HWND windowHandle = ServiceCentre::AccessApp()->GetHWND();

	// Initialise object shaders
	rasterizer = new Rasterizer(d3dDevice, windowHandle, L"VertPlotter.cso", L"Colorizer.cso");
	texturedRasterizer = new TexturedRasterizer(d3dDevice, windowHandle, L"VertPlotter.cso", L"Texturizer.cso");
	objectShaderCount = 1;

	// Initialise lighting shaders + store the default lighting
	// shader
	cookTorrancePBR = new CookTorrancePBR(d3dDevice, windowHandle, L"LightPlotter.cso", L"CookTorranceLighting.cso");
	lightingShaderCount = 1;
	currentLightingShader = defaultLightingShader;

	// Initialise the array of shadow shaders
	// No idea how to do shadows atm - come back to this after deferred rendering + lighting :)
	//availableShadowShaders = new Shader*[(byteUnsigned)AVAILABLE_SHADOW_SHADERS::NULL_SHADER];
	//availableShadowShaders[(byteUnsigned)AVAILABLE_SHADOW_SHADERS::PROCEDURAL_SHADOW_MAPPER] = new ProceduralShadowMapper(d3dDevice, windowHandle, L"ShaowPlotter.cso", L"ProceduralShadowPainter.cso");
	//shadowShaderCount = 1;

	// Initialise post-processing effects + define which post-processing
	// effects are active by default
	bloom = new Bloom(d3dDevice, windowHandle, L"PostEffectPlotter.cso", L"BloomEffect.cso");
	depthOfField = new DepthOfField(d3dDevice, windowHandle, L"PostEffectPlotter.cso", L"DepthOfFieldEffect.cso");
	activePostEffects[0] = defaultPostEffectA;
	activePostEffects[1] = defaultPostEffectB;
	activePostEffects[2] = defaultPostEffectC;
	postEffectCount = 0;

	// Initialise culling function pointers

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
	// Delete heap-allocated data within each object shader
	rasterizer->~Rasterizer();
	texturedRasterizer->~TexturedRasterizer();

	// Delete heap-allocated data within each lighting shader
	cookTorrancePBR->~CookTorrancePBR();

	// Delete heap-allocated data within each shadow shader
	//for (byteUnsigned i = 0; i < shadowShaderCount; i += 1)
	//{
	//	availableShadowShaders[i]->~Shader();
	//}

	// Delete heap-allocated data within each post-effect shader
	bloom->~Bloom();
	depthOfField->~DepthOfField();

	// Delete the render queue, then send it to [nullptr]
	delete renderQueue;
	renderQueue = nullptr;
}

void RenderManager::RasterizerRender(Material& renderableMaterial, ID3D11DeviceContext* deviceContext, DirectX::XMMATRIX& worldByModel, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection)
{
	rasterizer->Render(deviceContext, worldByModel, view, projection);
}

void RenderManager::TexturedRasterizerRender(Material& renderableMaterial, ID3D11DeviceContext* deviceContext, DirectX::XMMATRIX& worldByModel, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection)
{
	texturedRasterizer->Render(deviceContext, worldByModel, view, projection, renderableMaterial.GetTextureAsShaderResource());
}

void RenderManager::CookTorrancePBRRender(Material& renderableMaterial, ID3D11DeviceContext* deviceContext, DirectX::XMMATRIX& worldByModel, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection)
{
	cookTorrancePBR->Render(deviceContext, worldByModel, view, projection);
}

void RenderManager::ProceduralShadowMapperRenderer(Material& renderableMaterial, ID3D11DeviceContext* deviceContext, DirectX::XMMATRIX& worldByModel, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection)
{
	// Nothing for now...
}

void RenderManager::BloomRender(Material& renderableMaterial, ID3D11DeviceContext* deviceContext, DirectX::XMMATRIX& worldByModel, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection)
{
	// Nothing for now...
}

void RenderManager::DepthOfFieldRender(Material& renderableMaterial, ID3D11DeviceContext* deviceContext, DirectX::XMMATRIX& worldByModel, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection)
{
	// Nothing for now...
}

void RenderManager::Render(DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
{
	// Get each item from the render queue
	// Pass it to the GPU
	// Render the relevant shaders

	// Begin rendering by pushing the deferred render buffers onto the GPU
	BuffersToGPU();

	// Then get rid of any data from the last frame by flushing them with a
	// given color + clearing the depth buffer
	ClearBuffers(0.4f, 0.6f, 0.2f, 1.0f);

	// Render each boxecule within the render queue into the raster buffer
	for (fourByteSigned i = 0; i < renderQueueLength; i += 1)
	{
		Boxecule* renderable = renderQueue[i];
		AVAILABLE_OBJECT_SHADERS* shaders = renderable->GetMaterial().GetShaderSet();
		renderable->PassToGPU(deviceContext);

		byteUnsigned j = 0;
		while (shaders[j] != AVAILABLE_OBJECT_SHADERS::NULL_SHADER)
		{
			(this->*(this->objectShaderRenderDispatch[j]))(renderable->GetMaterial(), deviceContext, world * renderable->GetTransform(), view, projection);
			j += 1;
		}
	}

	// Apply lighting effects as appropriate for each pixel of the light buffer

	// Apply the currently-active post effects as appropriate for each pixel of the post buffer

	// Nothing left to draw on this pass, so zero the length of the render queue
	renderQueueLength = 0;
}

void RenderManager::BuffersToGPU()
{
	// Push the array of render buffers + the depth stencil buffer onto the GPU
	deviceContext->OMSetRenderTargets(RENDER_BUFFER_COUNT, renderBufferTargets, localDepthStencil);

	// Set the viewport
	//deviceContext->RSSetViewports(1, &m_viewport);
}

void RenderManager::ClearBuffers(float r, float g, float b, float a)
{
	// Flush the render buffers
	float color[4] = { r, g, b, a };
	deviceContext->ClearRenderTargetView(renderBufferTargets[(byteUnsigned)DEFERRED_BUFFER_TYPES::RASTER_BUFFER], color);
	deviceContext->ClearRenderTargetView(renderBufferTargets[(byteUnsigned)DEFERRED_BUFFER_TYPES::LIGHT_BUFFER], color);
	deviceContext->ClearRenderTargetView(renderBufferTargets[(byteUnsigned)DEFERRED_BUFFER_TYPES::POST_BUFFER], color);

	// Clear the local depth-stencil buffer (depth-stencil view)
	deviceContext->ClearDepthStencilView(localDepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

ID3D11ShaderResourceView* RenderManager::GetShaderResourceView(DEFERRED_BUFFER_TYPES deferredBuffer)
{
	return renderBufferShaderResources[(byteUnsigned)deferredBuffer];
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
	byteUnsigned dispatchIndex = (byteUnsigned)(boxecule->GetMaterial().GetColorData()[3] != 0);
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
