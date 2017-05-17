#pragma once

#include <d3d11.h>
#include <directxmath.h>

#include "AthruTexture2D.h"
#include "AthruTexture3D.h"

class Direct3D;
class Camera;
class Rasterizer;
class VoxelGrid;
class PostProcessor;
class ScreenRect;
class RenderManager
{
	public:
		RenderManager(ID3D11ShaderResourceView* postProcessShaderResource);
		~RenderManager();

		// Sample continuous galactic data into the scene texture, render the
		// voxel grid before coloring it with data sampled from the scene texture,
		// then pass the rendered data through a series of post-processing filters
		// before presenting the result to the screen
		void Render(Camera* mainCamera);

		// Retrieve a reference to the Direct3D handler class associated with [this]
		Direct3D* GetD3D();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// Helper function, used to sample continuous galactic data at the current
		// camera position into a scene texture that can be applied to the voxel
		// grid during rendering
		void PreProcess();

		// Helper function, used to render the voxel grid before coloring it with
		// data sampled from the scene texture during pre-processing (see above)
		void RenderScene(DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);

		// Helper function, used to perform post-processing and
		// draw the result to the screen
		void PostProcess(ScreenRect* screenRect,
						 DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);

		// Reference to the Direct3D handler class
		Direct3D* d3D;

		// Reference to the Direct3D device context
		ID3D11DeviceContext* d3dContext;

		// Voxel grid storage
		VoxelGrid* voxGrid;

		// References to scene color, normal, PBR, and emissivity textures
		AthruTexture3D sceneColorTexture;
		AthruTexture3D sceneNormalTexture;
		AthruTexture3D scenePBRTexture;
		AthruTexture3D sceneEmissivityTexture;

		// References to effects masks (used to adjust how post-processing effects e.g. blur,
		// brightening, and pattern filtering are applied to the scene after rendering)
		AthruTexture2D blurMask;
		AthruTexture2D brightenMask;
		AthruTexture2D drugsMask;

		// Shader storage
		Rasterizer* rasterizer;
		PostProcessor* postProcessor;
};
