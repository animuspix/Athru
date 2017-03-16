#pragma once

#include <d3d11.h>
#include <directxmath.h>
#include "Typedefs.h"
#include "DeferredRenderer.h"
#include "Shader.h"

enum class AVAILABLE_SHADERS
{
	RASTERIZER,
	PHONG_LIGHTING,
	COOK_TERRANCE_PBR,
	NULL_SHADER
};

class Camera;
class Boxecule;
class Chunk;
class RenderManager
{
	public:
		RenderManager(ID3D11DeviceContext* d3dDeviceContext, ID3D11Device* d3dDevice);
		~RenderManager();

		void Render(DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);

		// Prepare a series of boxecules for rendering, culling every item outside the main
		// camera's view frustum in the process
		void Prepare(Chunk** boxeculeChunks, Camera* mainCamera, twoByteUnsigned boxeculeDensity);

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void * target);

	private:
		// Do nothing; null function called if a boxecule processed during [Prepare(...)]
		// is invisible or positioned outside the camera frustum
		// Must have the same arguments as [boxeculeCache] so it can be stored within the
		// core boxecule-processing function pointer array (I figure an if-statement accessed
		// 70000+ times is a significant bottleneck, so replacing it with function pointers
		// and boolean magic should hopefully be a reasonable optimisation)
		bool BoxeculeDiscard(Boxecule* boxecule, fourByteUnsigned unculledCounter);

		// Store the given boxecule
		bool BoxeculeCache(Boxecule* boxecule, fourByteUnsigned unculledCounter);

		// Do nothing; null function called if a boxecule is completely behind the camera
		bool DirectionalCullFailed(Boxecule* boxecule, Camera* mainCamera, fourByteUnsigned unculledCounter);

		// Pass the given boxecule on to the core boxecule-processing functions
		// (called if a boxecule is in front of the camera position, regardless of whether
		// or not it's inside the camera frustum)
		bool DirectionalCullPassed(Boxecule* boxecule, Camera* mainCamera, fourByteUnsigned unculledCounter);

		// Each boxecule that makes it past initial chunk culling is directionally-tested, and the result is
		// used with [basicBoxeculeDispatch] to either call [DirectionalCullFailed(...)] or 
		// [DirectionalCulllPassed(...)]; boxecules that survive the directional cull are frustum-tested, and
		// the result is used with [coreBoxeculeDispatch] to either call [BoxeculeDiscard(...)] or 
		// [BoxeculeCache(...)]; [BoxeculeCache(...)] stores the given boxecule in the render queue and returns 
		// [true], while [BoxeculeDiscard(...)] does nothing and returns [false]. Both return values are echoed
		// through [DirectionalCullFailed(...)] and [DirectionalCullPassed(...)]
		
		// Do nothing; null function called if a chunk is outside the view triangle
		fourByteUnsigned ChunkDiscard(Chunk* chunk, bool isHome, twoByteUnsigned boxeculeDensity, Camera* mainCamera, fourByteUnsigned unculledCounter);

		// Pass the boxecules within the given chunk on to the basic boxecule-processing functions
		fourByteUnsigned ChunkCache(Chunk* chunk, bool isHome, twoByteUnsigned boxeculeDensity, Camera* mainCamera, fourByteUnsigned unculledCounter);

		// Helper function to segregate chunk directional-testing from generic boxecule testing/culling
		bool ChunkVisible(Chunk* chunk, Camera* mainCamera);

		ID3D11DeviceContext* deviceContext;
		Boxecule** renderQueue;
		DeferredRenderer* deferredRenderer;
		fourByteSigned renderQueueLength;
		Shader** availableShaders;

		// Chunk-processing function pointer array
		fourByteUnsigned(RenderManager::*chunkDispatch[2])(Chunk* chunk, bool isHome, twoByteUnsigned boxeculeDensity, Camera* mainCamera, fourByteUnsigned unculledCounter);

		// Low-fi boxecule-processing function pointer array
		bool(RenderManager::*basicBoxeculeDispatch[2])(Boxecule* boxecule, Camera* mainCamera, fourByteUnsigned unculledCounter);

		// Core boxecule-processing function pointer array
		bool(RenderManager::*coreBoxeculeDispatch[2])(Boxecule* boxecule, fourByteUnsigned unculledCounter);
};

