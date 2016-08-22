#include "engine.h"

// Consider remaking Engine/Input/Graphics as singleton services accessed through a
// Service Locator

// ...yuck. There's a LOT of refactorable gunk in here that should be cleaned out :|

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow)
{
	// Create the engine
	Engine* Athru = new Engine;

	if (Athru == false)
	{
		// Abandon ship if creation fails
		return 0;
	}

	// Start the engine
	Athru->Run();

	// When the engine stops running, wind everything down
	// and send the pointer accessing the engine to [nullptr]
	delete Athru;
	Athru = nullptr;

	return 0;
}