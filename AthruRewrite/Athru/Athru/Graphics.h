#pragma once

#include <windows.h>

#define FULL_SCREEN false
#define VSYNC_ENABLED true
#define SCREEN_DEPTH 1000.0f
#define SCREEN_NEAR 0.1f

class Graphics
{
	public:
		Graphics();
		//Graphics(int, int, HWND);
		~Graphics();

		bool Frame();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		bool Render();
};

