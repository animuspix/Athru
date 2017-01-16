#include "ServiceCentre.h"
#include "Application.h"

#include "leakChecker.h"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PSTR pScmdline, _In_ int iCmdshow)
{
	// Flag used to track memory leaks
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// Start the service centre
	ServiceCentre::StartUp();

	// Fetch the engine from the service centre, then start
	// it
	ServiceCentre::AccessApp()->Run();

	// Stop the service centre
	ServiceCentre::ShutDown();

	// Exit the application
	return 0;
}