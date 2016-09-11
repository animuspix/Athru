#pragma once

#include <windows.h>

#define FULL_SCREEN false
#define VSYNC_ENABLED true
#define SCREEN_DEPTH 1000.0f
#define SCREEN_NEAR 0.1f

#include "Service.h"

class Graphics : public Service
{
	public:
		Graphics();
		//Graphics(int, int, HWND);
		~Graphics();

		bool Frame();

	private:
		bool Render();
};

