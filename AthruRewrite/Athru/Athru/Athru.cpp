#include "ServiceCentre.h"
#include "Application.h"

#include "leakChecker.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow)
{
	// Flag used to track memory leaks
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// Create service centre here
	ServiceCentre::Create();

	// Fetch the engine from the service centre, then start
	// it
	((Application*)ServiceCentre::Instance().Fetch("Application"))->Run();

	// When the engine stops running, wind everything down
	// by destroying the service centre
	ServiceCentre::Destroy();

	// Exit the application
	return 0;
}