#pragma once

#include "Direct3D.h"
#include "Camera.h"
#include "Boxecule.h"
#include "Shader.h"
#include "RenderManager.h"

#define FULL_SCREEN false
#define VSYNC_ENABLED true
#define DISPLAY_WIDTH 1024
#define DISPLAY_HEIGHT 768
#define SCREEN_DEPTH 1000.0f
#define SCREEN_NEAR 0.1f

class Graphics
{
	public:
		Graphics(HWND hwnd, Logger* logger);
		~Graphics();

		void Frame();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		void Render();

		// Connection to the DirectX graphics processing backend
		Direct3D* d3D;

		// Connection to the player's camera
		Camera* camera;

		// An array of boxecule
		Boxecule* boxecule;

		// Pointer to the render manager
		RenderManager* renderManager;
};

