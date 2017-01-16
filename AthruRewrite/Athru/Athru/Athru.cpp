#include "ServiceCentre.h"
#include "Application.h"

#include "leakChecker.h"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PSTR pScmdline, _In_ int iCmdshow)
{
	// Flag used to track memory leaks
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// Create service centre here
	ServiceCentre::Create();

	// Fetch the engine from the service centre, then start
	// it
	ServiceCentre::Instance().AccessApp()->Run();

	// When the engine stops running, wind everything down
	// by destroying the service centre
	ServiceCentre::Destroy();

	// Exit the application
	return 0;
}