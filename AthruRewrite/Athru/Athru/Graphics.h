#pragma once

#include <windows.h>

const bool FULL_SCREEN = true;
const bool VSYNC_ENABLED = true;
const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.1f;

class Graphics
{
	public:
		Graphics();
		Graphics(int, int, HWND);
		~Graphics();

		bool Frame();

	private:
		bool Render();
};

