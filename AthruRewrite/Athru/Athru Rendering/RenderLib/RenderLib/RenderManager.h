#pragma once

#include <d3d11.h>
#include <directxmath.h>

#include "AthruTexture2D.h"
#include "AthruTexture3D.h"

class Direct3D;
class Camera;
class RayMarcher;
class PostProcessor;
class ScreenRect;
class RenderManager
{
	public:
		RenderManager(ID3D11ShaderResourceView* postProcessShaderResource);
		~RenderManager();

		// Pre-fill the scene texture data with the forms represented by the
		// given distance functions, then apply post-processing and render
		// the result to the display
		// No CPU-side distance functions for now; keep the process super-super simple
		// by doing as much of the work on the GPU as possible
		void Render(Camera* mainCamera,
					ID3D11ShaderResourceView* gpuReadableSceneDataView,
					ID3D11UnorderedAccessView* gpuWritableSceneDataView,
					fourByteUnsigned numSceneFigures);

		// Retrieve a reference to the Direct3D handler class associated with [this]
		Direct3D* GetD3D();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// Helper function, used to pre-fill the scene texture with ray-marched
		// color information
		void RenderScene(Camera* mainCamera,
						 ID3D11ShaderResourceView* gpuReadableSceneDataView,
						 ID3D11UnorderedAccessView* gpuWritableSceneDataView,
						 fourByteUnsigned numSceneFigures);

		// Helper function, used to perform post-processing and
		// draw the result to the screen
		void PostProcess(ScreenRect* screenRect,
						 DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);

		// Reference to the Direct3D handler class
		Direct3D* d3D;

		// Reference to the Direct3D device context
		ID3D11DeviceContext* d3dContext;

		// Reference to the scene raymarching/physics processing shader
		RayMarcher* rayMarcher;

		// Reference to the post-processing shader
		PostProcessor* postProcessor;
};
