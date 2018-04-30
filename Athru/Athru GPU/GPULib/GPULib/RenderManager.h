#pragma once

#include <d3d11.h>
#include <directxmath.h>

#include "AthruTexture2D.h"

class Direct3D;
class Camera;
class PathTracer;
class SceneFigure;
class ScreenPainter;
class ScreenRect;
class RenderManager
{
	public:
		RenderManager();
		~RenderManager();

		// Render the scene + apply post-processing
		void Render(Camera* mainCamera);

		// Retrieve a reference to the Direct3D handler class associated with [this]
		Direct3D* GetD3D();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// Helper function, used to pre-fill the scene texture with ray-marched
		// and post-processed color information
		void RenderScene(Camera* mainCamera);

		// Helper function, used to present the scene texture to the user by
		// projecting it onto a full-screen rectangle and using the graphics
		// pipeline to rasterize the result
		void Display(ScreenRect* screenRect);

		// Reference to the Direct3D handler class
		Direct3D* d3D;

		// Reference to the Direct3D device context
		const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& d3dContext;

		// Reference to the display texture (needed to migrate abstract
		// path-traced + post-processed data across to the screen)
		AthruTexture2D displayTex;

		// Reference to the scene path-tracing shader
		PathTracer* pathTracer;

		// Reference to the presentation shader
		ScreenPainter* screenPainter;
};
