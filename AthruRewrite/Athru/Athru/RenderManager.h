#pragma once

#include <d3d11.h>
#include <directxmath.h>
#include "Typedefs.h"
#include "DeferredRenderer.h"
#include "Shader.h"

#define RENDER_BUFFER_COUNT 3

enum class AVAILABLE_OBJECT_SHADERS
{
	RASTERIZER,
	TEXTURED_RASTERIZER,
	NULL_SHADER
};

enum class AVAILABLE_LIGHTING_SHADERS
{
	COOK_TORRANCE_PBR,
	NULL_SHADER
};

enum class AVAILABLE_SHADOW_SHADERS
{
	PROCEDURAL_SHADOW_MAPPER,
	NULL_SHADER
};

enum class AVAILABLE_POST_EFFECTS
{
	BLOOM,
	DEPTH_OF_FIELD,
	NULL_EFFECT
};

enum class DEFERRED_BUFFER_TYPES
{
	RASTER_BUFFER,
	LIGHT_BUFFER,
	POST_BUFFER
};

class Camera;
class Material;
class Boxecule;
class Chunk;
class SubChunk;
class Rasterizer;
class TexturedRasterizer;
class CookTorrancePBR;
class ProceduralShadowMapper;
class Bloom;
class DepthOfField;
class RenderManager
{
	public:
		RenderManager(ID3D11DeviceContext* d3dDeviceContext, ID3D11Device* d3dDevice, D3D11_VIEWPORT& viewport,
					  AVAILABLE_POST_EFFECTS postEffectA, AVAILABLE_POST_EFFECTS postEffectB, AVAILABLE_POST_EFFECTS postEffectC,
					  AVAILABLE_LIGHTING_SHADERS defaultLightingShader);
		~RenderManager();

		// Iterate through the contents of the render queue and pass each item to the GPU before
		// drawing it with the relevant shaders
		void Render(DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);

		// Prepare a series of boxecules for rendering, culling every item outside the main
		// camera's view frustum in the process
		void Prepare(Chunk** boxeculeChunks, Camera* mainCamera, twoByteUnsigned boxeculeDensity);

		// Get a shader-friendly version of the the texture output by the specified render-buffer
		// Useful for retrieving the output of a specific render-pass, e.g. rasterization,
		// lighting, post-processing, etc.
		ID3D11ShaderResourceView* GetShaderResourceView(DEFERRED_BUFFER_TYPES deferredBuffer);

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void * target);

	private:
		// Culling functions/function pointer arrays, accessed within [Prepare(...)]

		// Do nothing; null function called if a boxecule processed during [Prepare(...)]
		// is invisible or positioned outside the camera frustum
		// Must have the same arguments as [boxeculeCache] so it can be stored within the
		// core boxecule-processing function pointer array
		bool BoxeculeDiscard(Boxecule* boxecule, fourByteUnsigned unculledCounter);

		// Store the given boxecule inside the render queue
		bool BoxeculeCache(Boxecule* boxecule, fourByteUnsigned unculledCounter);

		// Do nothing; null function called if a boxecule is completely behind the camera
		bool DirectionalCullFailed(Boxecule* boxecule, Camera* mainCamera, fourByteUnsigned unculledCounter);

		// Pass the given boxecule on to the core boxecule-processing functions
		// (called if a boxecule is in front of the camera position, regardless of whether
		// or not it's inside the camera frustum)
		bool DirectionalCullPassed(Boxecule* boxecule, Camera* mainCamera, fourByteUnsigned unculledCounter);

		// Do nothing; null function called if a sub-chunk is outside the ZY view triangle
		fourByteUnsigned SubChunkDiscard(SubChunk* subChunk, bool withinHome, twoByteUnsigned boxeculeDensity, Camera* mainCamera, fourByteUnsigned unculledCounter);

		// Pass the boxecules within the given sub-chunk on to the basic boxecule-processing functions
		fourByteUnsigned SubChunkCache(SubChunk* subChunk, bool withinHome, twoByteUnsigned boxeculeDensity, Camera* mainCamera, fourByteUnsigned unculledCounter);

		// Do nothing; null function called if a chunk is outside the ZX view triangle
		fourByteUnsigned ChunkDiscard(Chunk* chunk, bool withinHome, twoByteUnsigned boxeculeDensity, Camera* mainCamera, fourByteUnsigned unculledCounter);

		// Pass the sub-chunks within the given chunk on to the relevant functions
		fourByteUnsigned ChunkCache(Chunk* chunk, bool isHome, twoByteUnsigned boxeculeDensity, Camera* mainCamera, fourByteUnsigned unculledCounter);

		// Sub-chunk processing function pointer array
		fourByteUnsigned(RenderManager::*subChunkDispatch[2])(SubChunk* subChunk, bool withinHome, twoByteUnsigned boxeculeDensity, Camera* mainCamera, fourByteUnsigned unculledCounter);

		// Chunk-processing function pointer array
		fourByteUnsigned(RenderManager::*chunkDispatch[2])(Chunk* chunk, bool isHome, twoByteUnsigned boxeculeDensity, Camera* mainCamera, fourByteUnsigned unculledCounter);

		// Low-fi boxecule-processing function pointer array
		bool(RenderManager::*basicBoxeculeDispatch[2])(Boxecule* boxecule, Camera* mainCamera, fourByteUnsigned unculledCounter);

		// Core boxecule-processing function pointer array
		bool(RenderManager::*coreBoxeculeDispatch[2])(Boxecule* boxecule, fourByteUnsigned unculledCounter);

		// Render functions + variables, accessed within [Render(...)]

		// Specialized shader render calls

		// Specialized render call for [TexturedRasterizer]
		void RasterizerRender(Material& renderableMaterial, ID3D11DeviceContext* deviceContext,
							  DirectX::XMMATRIX& world, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection,
							  fourByteUnsigned numIndicesDrawing);

		// Specialized render call for [Rasterizer]
		void TexturedRasterizerRender(Material& renderableMaterial, ID3D11DeviceContext* deviceContext,
									  DirectX::XMMATRIX& world, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection,
									  fourByteUnsigned numIndicesDrawing);

		// Specialized render call for [CookTorrancePBR]
		void CookTorrancePBRRender(ID3D11ShaderResourceView* rasterBuffer, ID3D11ShaderResourceView* lightBuffer, ID3D11DeviceContext* deviceContext,
								   DirectX::XMMATRIX& world, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection,
								   fourByteUnsigned numIndicesDrawing);

		// Specialized render call for [ProceduralShadowMapper]
		void ProceduralShadowMapperRenderer(Material& renderableMaterial, ID3D11DeviceContext* deviceContext,
											DirectX::XMMATRIX& world, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection,
											fourByteUnsigned numIndicesDrawing);

		// Specialized render call for [Bloom]
		void BloomRender(Material& renderableMaterial, ID3D11DeviceContext* deviceContext,
						 DirectX::XMMATRIX& world, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection,
						 fourByteUnsigned numIndicesDrawing);

		// Specialized render call for [DepthOfField]
		void DepthOfFieldRender(Material& renderableMaterial, ID3D11DeviceContext* deviceContext,
								DirectX::XMMATRIX& world, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection,
								fourByteUnsigned numIndicesDrawing);

		// Shader render dispatchers

		// Object shader render function pointer array, used to branchlessly (touch wood) call the
		// correct render function (with appropriate arguments) for any given shader applied to the given
		// boxecule
		void(RenderManager::*objectShaderRenderDispatch[(byteUnsigned)AVAILABLE_OBJECT_SHADERS::NULL_SHADER])(Material& renderableMaterial, ID3D11DeviceContext* deviceContext,
																											  DirectX::XMMATRIX& world, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection,
																											  fourByteUnsigned numIndicesDrawing);

		// Lighting shader render function pointer array, used to branchlessly (touch wood) call the
		// correct render function (with appropriate arguments) for any lighting shader applied to the scene
		void(RenderManager::*lightingShaderRenderDispatch[(byteUnsigned)AVAILABLE_LIGHTING_SHADERS::NULL_SHADER])(Material& renderableMaterial, ID3D11DeviceContext* deviceContext,
																												  DirectX::XMMATRIX& world, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection,
																												  fourByteUnsigned numIndicesDrawing);

		// Shadow shader render function pointer array, used to branchlessly (touch wood) call the
		// correct render function (with appropriate arguments) for any given shadow shader applied to the
		// scene
		void(RenderManager::*shadowShaderRenderDispatch[(byteUnsigned)AVAILABLE_SHADOW_SHADERS::NULL_SHADER])(Material& renderableMaterial, ID3D11DeviceContext* deviceContext,
																											  DirectX::XMMATRIX& world, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection,
																											  fourByteUnsigned numIndicesDrawing);

		// Post effect render function pointer array, used to branchlessly (touch wood) call the
		// correct render function (with appropriate arguments) for every post effect applied to the scene
		void(RenderManager::*postEffectRenderDispatch[(byteUnsigned)AVAILABLE_POST_EFFECTS::NULL_EFFECT])(Material& renderableMaterial, ID3D11DeviceContext* deviceContext,
																										  DirectX::XMMATRIX& world, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection);

		// Pass the deffered rendering buffers used by [this] to the GPU
		void BuffersToGPU();

		// Flush each deferred rendering buffer with the given color, then
		// clear the depth stencil buffer
		void ClearBuffers(float r, float g, float b, float a);

		// Reference to the DirectX device context
		ID3D11DeviceContext* deviceContext;

		// Lots of references to "views" below; they're a fairly abstract DirectX concept
		// that's kinda hard to explain, but there's an excellent description from StackOverflow
		// available over here:
		// http://stackoverflow.com/questions/20106440/what-is-a-depth-stencil-view#20130381

		// Array of textures used as render targets during deferred rendering
		// Render targets are basically chunks of in-game image data that can be used
		// as output locations by Direct3D; the default render target is the screen,
		// but any chunk of image data (e.g. a texture) can be used instead
		ID3D11Texture2D* renderBufferTextures[RENDER_BUFFER_COUNT];

		// [renderBuffers] interpreted as an array of render targets
		// rather than an array of textures
		ID3D11RenderTargetView* renderBufferTargets[RENDER_BUFFER_COUNT];

		// [renderBuffers] interpreted as an array of shader resources; this
		// version is explicitly designed for shader i/o, whereas the others are more
		// abstract models of the render-buffers used in the deferred rendering process
		ID3D11ShaderResourceView* renderBufferShaderResources[RENDER_BUFFER_COUNT];

		// Texture representing the depth stencil accessed during deferred rendering
		// (while drawing to the raster buffer)
		ID3D11Texture2D* localDepthStencilTexture;

		// [localDepthStencilTexture] interpreted as a depth-stencil object rather than a
		// texture
		ID3D11DepthStencilView* localDepthStencil;

		// Reference to the Direct3D viewport accessed by [this]
		D3D11_VIEWPORT& viewportRef;

		// Render-item storage + length tracker
		Boxecule** renderQueue;
		fourByteSigned renderQueueLength;

		// Object shader storage + length tracker
		//Shader** availableObjectShaders;
		Rasterizer* rasterizer;
		TexturedRasterizer* texturedRasterizer;
		byteUnsigned objectShaderCount;

		// Lighting shader storage + length tracker
		CookTorrancePBR* cookTorrancePBR;
		byteUnsigned lightingShaderCount;

		// The current lighting model (Cook-Torrance PBR, Phong, etc)
		AVAILABLE_LIGHTING_SHADERS currentLightingShader;

		// Shadow shader storage + length tracker
		// --no defined shadow shaders atm--
		byteUnsigned shadowShaderCount;

		// Post-processing effect storage + length tracker
		Bloom* bloom;
		DepthOfField* depthOfField;
		byteUnsigned postEffectCount;

		// Array of active post-processing effects
		AVAILABLE_POST_EFFECTS activePostEffects[3];
};
