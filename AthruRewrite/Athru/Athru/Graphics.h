#pragma once

#include "Direct3D.h"
#include "Camera.h"
#include "Triangle.h"
#include "Shaders.h"

#define FULL_SCREEN false
#define VSYNC_ENABLED true
#define SCREEN_DEPTH 1000.0f
#define SCREEN_NEAR 0.1f

#define MAX_VISIBLE_BOXECULES 10000

class Graphics
{
	public:
		Graphics(int screenWidth, int screenHeight, HWND hwnd, Logger* logger);
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

		// Array of pointers to currently visible Boxecules (refactor to use a
		// stack-based chunking system)
		Triangle* triangle;

		// Pointer to the shader manager
		Shaders* shaderManager;
};

