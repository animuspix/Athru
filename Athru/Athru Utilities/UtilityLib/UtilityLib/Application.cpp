#include "UtilityServiceCentre.h"
#include "Application.h"

Application::Application()
{
	// Create the window
	BuildWindow(GraphicsStuff::DISPLAY_WIDTH, GraphicsStuff::DISPLAY_HEIGHT);

	// Create the message structure + zero it's contents
	// to prevent any pre-window-creation messages from being
	// processed
	msg = MSG();
	ZeroMemory(&msg, sizeof(MSG));
}

Application::~Application()
{
	// Close the window
	CloseWindow();
}

void Application::RelayOSMessages()
{
	// Remove older messages from memory by zeroing
	// the contents of the message structure
	ZeroMemory(&msg, sizeof(MSG));

	// Handle OS messages with [WndProc]
	if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

// Primary message handler; processes input messages received from the OS and
// responds appropriately
LRESULT CALLBACK Application::WndProc(HWND windowHandle, UINT message, WPARAM messageParamA, LPARAM messageParamB)
{
	// Cache a local reference to the input handler
	Input* localInput = AthruCore::Utility::AccessInput();

	switch (message)
	{
		// Check if the window is being destroyed.
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}

		// Check if the window is being closed.
		case WM_CLOSE:
		{
			PostQuitMessage(0);
			return 0;
		}

		// Check if either WM_CLOSE or WM_DESTROY ran successfully
		// and nullified the window handle
		case WM_QUIT:
		{
			localInput->SetCloseFlag();
			return 0;
		}

		// Check if a key has been pressed on the keyboard
		case WM_KEYDOWN:
		{
			// If a key is pressed, send it to the input object so it can record that state
			localInput->KeyDown((u4Byte)messageParamA);
			return 0;
		}

		// Check if a key has been released on the keyboard
		case WM_KEYUP:
		{
			// If a key is released, send it to the input object so it can unset the state for that key
			localInput->KeyUp((u4Byte)messageParamA);
			return 0;
		}

		// Check for mouse movement
		case WM_MOUSEMOVE:
		{
			// If the mouse has moved, store its coordinates in the input handler
			POINT mousePoint;
			GetCursorPos(&mousePoint);
			ScreenToClient(windowHandle, &mousePoint);
			localInput->CacheMousePos((float)mousePoint.x,
									  (float)mousePoint.y);
		}

		// Check if the left-mouse-button has been pressed
		case WM_LBUTTONDOWN:
		{
			// If the left-mouse-button has been pressed, cache that state-change
			// within the input object so that it's visible to the rest of the
			// program
			localInput->LeftMouseDown();
		}

		// Check if the left-mouse-button has been released
		case WM_LBUTTONUP:
		{
			// If the left-mouse-button has been released, cache that state-change
			// within the input object so that it's visible to the rest of the
			// program
			localInput->LeftMouseUp();
		}

		// Ignore any messages that haven't already been handled
		default:
		{
			return DefWindowProc(windowHandle, message, messageParamA, messageParamB);
		}
	}
}

void Application::BuildWindow(const int& windowedWidth, const int& windowedHeight)
{
	WNDCLASSEX wc;
	DEVMODE dmScreenSettings;

	// Get the instance of this application.
	appInstance = GetModuleHandle(NULL);

	// Give the application a name.
	appName = L"Athru";

	// Setup the windows class with default settings.
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = appInstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = appName;
	wc.cbSize = sizeof(WNDCLASSEX);

	// Register the window class.
	RegisterClassEx(&wc);

	// Determine the resolution of the clients desktop screen.
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	// Setup the screen settings depending on whether it is running in full screen or in windowed mode.
	if (GraphicsStuff::FULL_SCREEN)
	{
		// If full screen set the screen to maximum size of the users desktop and 32bit.
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth = (unsigned long)screenWidth;
		dmScreenSettings.dmPelsHeight = (unsigned long)screenHeight;
		dmScreenSettings.dmBitsPerPel = 32;
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		// Change the display settings to full screen.
		ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

		// Set the position of the window to the top left corner.
		int posX = 0;
		int posY = 0;

		// Create the window with the screen settings and get the handle to it.
		appForm = CreateWindowEx(WS_EX_APPWINDOW, appName, appName,
				  WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP, posX, posY, screenWidth,
				  screenHeight, NULL, NULL, appInstance, NULL);
	}
	else
	{
		// Place the window in the middle of the screen.
		int posX = (screenWidth - windowedWidth) / 2;
		int posY = (screenHeight - windowedHeight) / 2;

		// Create the window with the screen settings and get the handle to it.
		appForm = CreateWindowEx(WS_EX_APPWINDOW, appName, appName,
			      WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP, posX, posY, windowedWidth,
			      windowedHeight, NULL, NULL, appInstance, NULL);
	}

	// Bring the window up on the screen and set it as main focus.
	ShowWindow(appForm, SW_SHOW);
	SetForegroundWindow(appForm);
	SetFocus(appForm);

	// Hide the mouse cursor.
	ShowCursor(false);
}

void Application::CloseWindow()
{
	// Show the mouse cursor.
	ShowCursor(true);

	// Fix the display settings if leaving full screen mode.
	if (GraphicsStuff::FULL_SCREEN)
	{
		ChangeDisplaySettings(NULL, 0);
	}

	// Remove the window.
	DestroyWindow(appForm);
	appForm = NULL;

	// Remove the application instance.
	UnregisterClass(appName, appInstance);
	appInstance = NULL;
}

HWND Application::GetHWND()
{
	return appForm;
}

// Push constructions for this class through Athru's custom allocator
void* Application::operator new(size_t size)
{
	StackAllocator* allocator = AthruCore::Utility::AccessMemory();
	return allocator->AlignedAlloc(size, (uByte)std::alignment_of<Application>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void Application::operator delete(void* target)
{
	return;
}
