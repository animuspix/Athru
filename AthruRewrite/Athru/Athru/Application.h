#pragma once

#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "Service.h"

class Input;
class Graphics;
class Logger;

class Application : public Service
{
	public:
		// Build the engine
		Application();

		// Delete the engine
		~Application();

		// Control the game loop
		void Run();

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