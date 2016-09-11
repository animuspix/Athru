#include "ServiceCentre.h"
#include "Input.h"
#include "Graphics.h"
#include "Application.h"

Application::Application() : Service("Application")
{
	// Create the window
	BuildWindow(1024, 768);
}

bool Application::Frame()
{
	// Exit the frame and return [false] (ending the game loop)
	// if the player presses [Escape]
	if (athruInput->IsKeyDown(VK_ESCAPE))
	{
		return false;
	}

	// Process the current frame through the Graphics class
	bool result = (athruGraphics->Frame());
	if (!result)
	{
		// If the current frame returns false (the player quit,
		// something went wrong, etc.), end the game loop
		return false;
	}

	// If everything worked, return [true]
	return true;
}

void Application::Run()
{
	MSG msg;
	bool done, result;

	// Initialize the message structure.
	ZeroMemory(&msg, sizeof(MSG));

	// Generate private references to the input and graphics services
	// over here, since they can't be generated in the constructor
	// (the application constructor is called by the Service centre
	// within ITS constructor, so generating these references there
	// is a guaranteed way to create a dependency loop :P)
	athruInput = ((Input*)(ServiceCentre::Instance().Fetch("Input")));
	athruGraphics = ((Graphics*)(ServiceCentre::Instance().Fetch("Graphics")));

	// Loop until there is a quit message from the window or the user.
	done = false;
	while (!done)
	{
		// Handle the windows messages.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// If windows signals to end the application then exit out.
		if (msg.message == WM_QUIT)
		{
			done = true;
		}
		else
		{
			// Otherwise do the frame processing.
			result = Frame();
			if (!result)
			{
				done = true;
			}
		}
	}
}

// Primary message handler; processes input messages received from the OS and
// responds appropriately
LRESULT CALLBACK Application::WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
	switch (umessage)
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

		// Check if a key has been pressed on the keyboard.
		case WM_KEYDOWN:
		{
			// If a key is pressed send it to the input object so it can record that state.
			((Input*)ServiceCentre::Instance().Fetch("Input"))->KeyDown((unsigned int)wparam);
			return 0;
		}

		// Check if a key has been released on the keyboard.
		case WM_KEYUP:
		{
			// If a key is released then send it to the input object so it can unset the state for that key.
			((Input*)ServiceCentre::Instance().Fetch("Input"))->KeyUp((unsigned int)wparam);
			return 0;
		}

		// Any other messages send to the default message handler as our application won't make use of them.
		default:
		{
			return DefWindowProc(hwnd, umessage, wparam, lparam);;
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
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = appName;
	wc.cbSize = sizeof(WNDCLASSEX);

	// Register the window class.
	RegisterClassEx(&wc);

	// Determine the resolution of the clients desktop screen.
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	// Setup the screen settings depending on whether it is running in full screen or in windowed mode.
	if (FULL_SCREEN)
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
	if (FULL_SCREEN)
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

Application::~Application()
{
	// Close the window
	CloseWindow();
}
