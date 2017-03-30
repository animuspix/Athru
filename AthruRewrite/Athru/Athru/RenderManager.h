#pragma once

#include <d3d11.h>
#include <directxmath.h>
#include "Typedefs.h"
#include "Shader.h"

#define MAX_POINT_LIGHT_COUNT 16
#define MAX_SPOT_LIGHT_COUNT 16
#define MAX_DIRECTIONAL_LIGHT_COUNT 1

enum class AVAILABLE_OBJECT_SHADERS
{
	RASTERIZER,
	TEXTURED_RASTERIZER,
	SPHERICAL_RASTERIZER,
	TEXTURED_SPHERICAL_RASTERIZER,
	NULL_SHADER
};

enum class AVAILABLE_LIGHTING_SHADERS
{
	DIRECTIONAL_LIGHT,
	POINT_LIGHT,
	SPOT_LIGHT,
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

class Direct3D;
class Camera;
class Material;
class Boxecule;
class Chunk;
class SubChunk;
class Rasterizer;
class TexturedRasterizer;
class DirectionalLight;
class PointLight;
class SpotLight;
class ProceduralShadowMapper;
class Bloom;
class DepthOfField;
class RenderManager
{
	public:
		RenderManager(ID3D11DeviceContext* d3dDeviceContext, ID3D11Device* d3dDevice,
					  AVAILABLE_POST_EFFECTS postEffectA, AVAILABLE_POST_EFFECTS postEffectB, AVAILABLE_POST_EFFECTS postEffectC);
		~RenderManager();

		// Iterate through the contents of the render queue and pass each item to the GPU before
		// drawing it with the relevant shaders
		void Render(Camera* mainCamera,
					DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);

		// Prepare a series of boxecules for rendering, culling every item outside the main
		// camera's view frustum in the process
		void Prepare(Chunk** boxeculeChunks, Camera* mainCamera, twoByteUnsigned boxeculeDensity);

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void * target);

	private:
		// Culling functions/function pointer arrays, accessed within [Prepare(...)]

		// Store the render-queue index of a light with non-zero-intensity within [visibleLights]
		void LightCache(fourByteUnsigned boxeculeIndex);

		// Do nothing; null function called if a boxecule has zero lighting intensity
		void LightDiscard(fourByteUnsigned boxeculeIndex);

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

		// Luminance-processing function pointer array
		void(RenderManager::*lightingShaderFilterDispatch[2])(fourByteUnsigned boxeculeIndex);

		// Render functions + variables, accessed within [Render(...)]
		ID3D11DeviceContext* deviceContext;

		// Specialized shader render calls

		// Specialized render call for [TexturedRasterizer]
		void RasterizerRender(Material& renderableMaterial, Material& directionalMaterial,
							  Material* pointLightMaterials, Material* spotLightMaterials,
							  DirectX::XMVECTOR& directionalPosition, DirectX::XMVECTOR& directionalRotationQtn,
							  DirectX::XMVECTOR* pointLightPositions, DirectX::XMVECTOR* pointLightRotationQtns,
							  fourByteUnsigned numPointLights,
							  DirectX::XMVECTOR* spotLightPositions, DirectX::XMVECTOR* spotLightRotationQtns,
							  fourByteUnsigned numSpotLights,
							  DirectX::XMMATRIX& worldByModel, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection,
							  ID3D11DeviceContext* deviceContext,
							  fourByteUnsigned numIndicesDrawing);

		// Specialized render call for [ProceduralShadowMapper]
		void ProceduralShadowMapperRenderer(Material& renderableMaterial,
											ID3D11DeviceContext* deviceContext,
											DirectX::XMMATRIX& world, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection,
											fourByteUnsigned numIndicesDrawing);

		// Specialized render call for [Bloom]
		void BloomRender(ID3D11ShaderResourceView* rasterBuffer,
						 ID3D11ShaderResourceView* lightBuffer,
						 ID3D11ShaderResourceView* postBuffer,
						 ID3D11DeviceContext* deviceContext,
						 DirectX::XMMATRIX& world, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection,
						 fourByteUnsigned numIndicesDrawing);

		// Specialized render call for [DepthOfField]
		void DepthOfFieldRender(ID3D11ShaderResourceView* rasterBuffer,
								ID3D11ShaderResourceView* lightBuffer,
								ID3D11ShaderResourceView* postBuffer,
								ID3D11DeviceContext* deviceContext,
								DirectX::XMMATRIX& world, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection,
								fourByteUnsigned numIndicesDrawing);

		// Shader render dispatchers

		// Object shader render function pointer array, used to branchlessly (touch wood) call the
		// correct render function (with appropriate arguments) for any given shader accessed by the
		// given material
		void(RenderManager::*objectShaderRenderDispatch[(byteUnsigned)AVAILABLE_OBJECT_SHADERS::NULL_SHADER])(Material& renderableMaterial,
																											  ID3D11DeviceContext* deviceContext,
																											  DirectX::XMMATRIX& world, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection,
																											  fourByteUnsigned numIndicesDrawing);

		// Lighting shader render function pointer array, used to branchlessly (touch wood) call the
		// correct render function (with appropriate arguments) for any lighting shader described in
		// [lightData] and emanating from [lightPosition]
		void(RenderManager::*lightingShaderRenderDispatch[(byteUnsigned)AVAILABLE_LIGHTING_SHADERS::NULL_SHADER])(Material& emitterMaterial,
																												  DirectX::XMVECTOR& lightPosition,
																												  DirectX::XMVECTOR& lightRotationQtn,
																												  DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection,
																												  ID3D11DeviceContext* deviceContext,
																												  fourByteUnsigned numIndicesDrawing);

		// Shadow shader render function pointer array, used to branchlessly (touch wood) call the
		// correct render function (with appropriate arguments) for any given shadow shader applied to the
		// scene
		void(RenderManager::*shadowShaderRenderDispatch[(byteUnsigned)AVAILABLE_SHADOW_SHADERS::NULL_SHADER])(Material& renderableMaterial,
																											  ID3D11DeviceContext* deviceContext,
																											  DirectX::XMMATRIX& world, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection,
																											  fourByteUnsigned numIndicesDrawing);

		// Post effect render function pointer array, used to branchlessly (touch wood) call the
		// correct render function (with appropriate arguments) for every post effect applied to the scene
		void(RenderManager::*postEffectRenderDispatch[(byteUnsigned)AVAILABLE_POST_EFFECTS::NULL_EFFECT])(ID3D11ShaderResourceView* rasterBuffer,
																										  ID3D11ShaderResourceView* lightBuffer,
																										  ID3D11ShaderResourceView* postBuffer,
																										  ID3D11DeviceContext* deviceContext,
																										  DirectX::XMMATRIX& world, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection,
																										  fourByteUnsigned numIndicesDrawing);

		// Render-item storage + length tracker
		Boxecule** renderQueue; // Big box of things to render, not actually a queue ("queue" sounds nicer than "stack" tho :P)
		fourByteSigned renderQueueLength; // Total number of things to render
		fourByteUnsigned* visibleLights; // Fake associative array; stores the indices of active lights within [renderQueue]
		fourByteUnsigned visibleLightCount; // Store the number of active lights within [renderQueue]

		// Object shader storage + length tracker
		Rasterizer* rasterizer;
		TexturedRasterizer* texturedRasterizer;
		byteUnsigned objectShaderCount;

		// Lighting shader storage
		DirectionalLight* directionalLight;
		PointLight* pointLight;
		SpotLight* spotLight;

		// Shadow shader storage + length tracker
		// --no defined shadow shaders atm--
		byteUnsigned shadowShaderCount;

		// Post-processing effect storage + length tracker
		Bloom* bloom;
		DepthOfField* depthOfField;
		byteUnsigned postEffectCount;
};
