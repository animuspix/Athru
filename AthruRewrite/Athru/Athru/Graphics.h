#pragma once

#include "Direct3D.h"

#define FULL_SCREEN false
#define VSYNC_ENABLED true
#define SCREEN_DEPTH 1000.0f
#define SCREEN_NEAR 0.1f

class Graphics
{
	public:
		Graphics(int screenWidth, int screenHeight, HWND hwnd);
		~Graphics();

		void Frame();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		void Render();

		// Connection to the DirectX graphics processing backend
		Direct3D d3D;
};

