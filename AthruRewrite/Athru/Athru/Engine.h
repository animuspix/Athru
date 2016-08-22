#pragma once

#define WIN32_LEAN_AND_MEAN
#include "windows.h"

class Input;
class Graphics;

class Engine
{
	public:
		// Build the engine
		Engine();

		// Delete the engine
		~Engine();

		// Control the game loop
		void Run();

		// Process system messages (mainly key/mouse input so that
		// they can be delegated to the input manager)
		LRESULT CALLBACK MessageHandler(HWND, UINT, WPARAM, LPARAM);

	private:
		// Handle individual frames through the Graphics class
		bool Frame();

		// Initialise game window
		void BuildWindow(int& screenWidth, int& screenHeight);

		// Close game window
		void CloseWindow();

		LPCWSTR m_applicationName;
		HINSTANCE m_hinstance;
		HWND m_hwnd;

		// Pointer for accessing user input
		Input* m_pInput;

		// Pointer to access graphics processes + calculations
		Graphics* m_pGraphics;
};

/////////////////////////
// FUNCTION PROTOTYPES //
/////////////////////////
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


/////////////
// GLOBALS //
/////////////
static Engine* ApplicationHandle = 0;