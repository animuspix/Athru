#pragma once

#define WIN32_LEAN_AND_MEAN
#include "windows.h"

class Input;
class Graphics;
class Logger;

class Application
{
	public:
		// Build the engine
		Application();

		// Delete the engine
		~Application();

		// Control the game loop
		void Run();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// Handle individual frames through the Graphics/Input classes
		bool Frame();

		// Initialise game window
		void BuildWindow(const int& screenWidth, const int& screenHeight);

		// Close game window
		void CloseWindow();

		// Stored pointers to input, graphics, and the logger, removing the
		// need to repeatedly fetch them from the service centre
		Input* athruInput;
		Graphics* athruGraphics;
		Logger* athruLogger;

		// Name of the application
		LPCWSTR appName;

		// Handle for the current instance of the application, used by Windows
		// to track the application within the system overall
		HINSTANCE appInstance;

		// Value used by Windows to identify the interactive aspect of the
		// application (referred to as a "form" within Windows itself)
		HWND appForm;

		// Process input messages provided by the OS and respond
		// appropriately
		static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
};