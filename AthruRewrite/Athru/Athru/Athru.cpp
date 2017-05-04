#include "HiLevelServiceCentre.h"
#include "Application.h"
#include "leakChecker.h"

void GameLoop()
{
	Application* athruApp = AthruUtilities::UtilityServiceCentre::AccessApp();
	Input* athruInput = AthruUtilities::UtilityServiceCentre::AccessInput();
	Graphics* athruGraphics = HiLevelServiceCentre::AccessGraphics();

	bool gameExiting = false;
	while (!gameExiting)
	{
		// Catch HID messages from the OS
		athruApp->RelayOSMessages();

		// Check for closing conditions
		bool localCloseFlag = athruInput->IsKeyDown(VK_ESCAPE) || athruInput->GetCloseFlag();
		if (localCloseFlag)
		{
			gameExiting = true;
			break;
		}

		// Perform update/draw operations here
		athruGraphics->Update();
		athruGraphics->Draw();
	}
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PSTR pScmdline, _In_ int iCmdshow)
{
	// Flag used to track memory leaks
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// Start the service centre
	HiLevelServiceCentre::StartUp();

	// Start the game loop
	GameLoop();

	// When the game loop stops, clean everything up by
	// stopping the service centre as well
	HiLevelServiceCentre::ShutDown();

	// Exit the application
	return 0;
}