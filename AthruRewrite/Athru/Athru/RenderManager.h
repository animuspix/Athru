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
class RenderManager
{
	public:
		RenderManager(ID3D11DeviceContext* d3dDeviceContext, ID3D11Device* d3dDevice);
		~RenderManager();

		void Render(DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);

		// Prepare a series of boxecules for rendering, culling every item outside the main
		// camera's view frustum in the process
		void Prepare(Boxecule** boxeculeSet, Camera* mainCamera);

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

		// Each preparation stage enters into [directional]

		ID3D11DeviceContext* deviceContext;
		Boxecule** renderQueue;
		DeferredRenderer* deferredRenderer;
		fourByteSigned renderQueueLength;
		Shader** availableShaders;

		// Low-fi boxecule-processing function pointer array
		bool(RenderManager::*basicBoxeculeDispatch[2])(Boxecule* boxecule, Camera* mainCamera, fourByteUnsigned unculledCounter);

		// Core boxecule-processing function pointer array
		bool(RenderManager::*coreBoxeculeDispatch[2])(Boxecule* boxecule, fourByteUnsigned unculledCounter);
};

