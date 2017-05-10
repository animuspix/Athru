#pragma once

#include <d3d11.h>
#include <directxmath.h>

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
class Boxecule;
class AthruRect;
class Rasterizer;
class PostProcessor;
class RenderManager
{
	public:
		RenderManager(ID3D11ShaderResourceView* postProcessShaderResource);
		~RenderManager();

		// Draw the scene grid onto the screen texture, then color it with the final scene data
		void Render(Camera* mainCamera);

		// Perform post-processing and draw the result to the screen
		void PostProcess(AthruRect* screenRect,
					     DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);

		// Retrieve a reference to the Direct3D handler class associated with [this]
		Direct3D* GetD3D();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void * target);

	private:
		// Reference to the Direct3D handler class
		Direct3D* d3D;

		// Shader storage
		Rasterizer* rasterizer;
		PostProcessor* postProcessor;
};
