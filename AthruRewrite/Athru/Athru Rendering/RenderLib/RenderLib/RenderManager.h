#pragma once

#include <d3d11.h>
#include <directxmath.h>
#include "AthruGlobals.h"

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
class AthruRect;
class Chunk;
class SubChunk;
class Rasterizer;
class PostProcessor;
class RenderManager
{
	public:
		RenderManager(ID3D11DeviceContext* d3dDeviceContext, ID3D11Device* d3dDevice,
					  AVAILABLE_POST_EFFECTS postEffectA, AVAILABLE_POST_EFFECTS postEffectB,
					  ID3D11ShaderResourceView* postProcessShaderResource);
		~RenderManager();

		// Iterate through the contents of the render queue and pass each item to the GPU before
		// drawing it with the relevant shaders
		// Should only run after the screen texture has been passed onto the pipeline; otherwise
		// the post-processing shader will lose access to the actual scene data
		void Render(Camera* mainCamera,
					DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);

		// Perform post-processing and draw the result to the screen
		void PostProcess(AthruRect* screenRect,
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

		// Pass the boxecules within the given sub-chunk on to the basic boxecule-processing functions
		fourByteUnsigned SubChunkCache(SubChunk* subChunk, bool withinHome, twoByteUnsigned boxeculeDensity, Camera* mainCamera, fourByteUnsigned unculledCounter);

		// Pass the sub-chunks within the given chunk on to the relevant functions
		fourByteUnsigned ChunkCache(Chunk* chunk, bool isHome, twoByteUnsigned boxeculeDensity, Camera* mainCamera, fourByteUnsigned unculledCounter);

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

		// Shader render dispatchers

		// Object shader render function pointer array, used to branchlessly (touch wood) call the
		// correct render function (with appropriate arguments) for any given shader accessed by the
		// given material
		void(RenderManager::*objectShaderRenderDispatch[(byteUnsigned)AVAILABLE_OBJECT_SHADERS::NULL_SHADER])(Material& renderableMaterial,
																											  ID3D11DeviceContext* deviceContext,
																											  DirectX::XMMATRIX& world, DirectX::XMMATRIX& view, DirectX::XMMATRIX& projection,
																											  fourByteUnsigned numIndicesDrawing);

		// Render-item storage + length tracker
		Boxecule** renderQueue; // Big box of things to render, not actually a queue ("queue" sounds nicer than "stack" tho :P)
		fourByteSigned renderQueueLength; // Total number of things to render
		fourByteUnsigned* visibleLights; // Fake associative array; stores the indices of active lights within [renderQueue]
		fourByteUnsigned visibleLightCount; // Store the number of active lights within [renderQueue]

		// Shader storage
		Rasterizer* rasterizer;
		PostProcessor* postProcessor;
};
