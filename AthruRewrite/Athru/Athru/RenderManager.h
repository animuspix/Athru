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
		ID3D11DeviceContext* deviceContext;
		Boxecule** renderQueue;
		DeferredRenderer* deferredRenderer;
		fourByteSigned renderQueueLength;
		Shader** availableShaders;
};

