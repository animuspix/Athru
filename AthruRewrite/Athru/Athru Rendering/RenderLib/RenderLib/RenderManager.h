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

		// Draw the scene grid onto the screen texture, then color it with the final scene data
		void Render(Camera* mainCamera,
					DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);

		// Perform post-processing and draw the result to the screen
		void PostProcess(AthruRect* screenRect,
					     DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);

		// Retrieve a reference to the Direct3D device interface accessed by [this]
		ID3D11Device* GetD3DDevice();

		// Retrieve a reference to the Direct3D device-context interface accessed by [this]
		ID3D11DeviceContext* GetD3DDeviceContext();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void * target);

	private:
		// References to the Direct3D device/device-context interfaces, needed for draw calls, mesh setup,
		// etc
		ID3D11Device* device;
		ID3D11DeviceContext* deviceContext;

		// Culling functions/function pointer arrays, accessed within [Prepare(...)]

		// Specialized shader render calls

		// Shader storage
		Rasterizer* rasterizer;
		PostProcessor* postProcessor;
};
