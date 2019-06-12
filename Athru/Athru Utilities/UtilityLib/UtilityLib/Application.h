#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>

class Input;

class Application
{
	public:
		// Build the engine
		Application();

		// Delete the engine
		~Application();

		// Pass OS messages along to [Input]
		void RelayOSMessages();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

		// Retrieve the window handle
		HWND GetHWND();

		// Retrieve the monitor dimensions (allows fullscreen on arbitrary system monitors)
		void GetMonitorRes(u4Byte* x, u4Byte* y);
	private:
		// Initialise game window
		void BuildWindow(const int& screenWidth, const int& screenHeight);

		// Close game window
		void CloseWindow();

		// Name of the application
		LPCWSTR appName;

		// Handle for the current instance of the application, used by Windows
		// to track the application within the system overall
		HINSTANCE appInstance;

		// Value used by Windows to identify the interactive aspect of the
		// application (referred to as a "form" within Windows itself)
		HWND appForm;

		// Size of the output monitor
		u4Byte monitorRes[2];

		// A windows message structure; used for storing formatted copies of
		// raw messages received from the OS event queue
		MSG msg;

		// Process input messages provided by the OS and respond
		// appropriately
		static LRESULT CALLBACK WndProc(HWND windowHandle, UINT message, WPARAM messageParamA, LPARAM messageParamB);
};