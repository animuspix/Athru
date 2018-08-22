#include "HiLevelServiceCentre.h"

void GameLoop()
{
	// Cache local references to utility services
	Application* athruApp = AthruUtilities::UtilityServiceCentre::AccessApp();
	Input* athruInput = AthruUtilities::UtilityServiceCentre::AccessInput();

	// Cache a local reference to the GPU messenger object
	GPUMessenger* athruGPUMessenger = AthruGPU::GPUServiceCentre::AccessGPUMessenger();

	// Cache a local reference to the render-manager
	Renderer* athruRendering = AthruGPU::GPUServiceCentre::AccessRenderer();

	// Cache a local reference to the GPU update manager
	//GPUUpdateManager* athruGPUUpdates = AthruGPU::GPUServiceCentre::AccessGPUUpdateManager();

	// Cache a local reference to the high-level scene representation
	Scene* athruScene = HiLevelServiceCentre::AccessScene();

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

		// Log CPU-side FPS
		//AthruUtilities::UtilityServiceCentre::AccessLogger()->Log("Logging CPU-side FPS", Logger::DESTINATIONS::CONSOLE);
		//AthruUtilities::UtilityServiceCentre::AccessLogger()->Log(TimeStuff::FPS(), Logger::DESTINATIONS::CONSOLE);

		// Update the game, push current scene to the GPU
		athruScene->Update();

		// Pass scene data to the GPU for updates/rendering
		athruGPUMessenger->SceneToGPU(AthruGPU::GPUServiceCentre::AccessD3D()->GetDeviceContext(),
									  athruScene->CollectLocalFigures());

		// Perform GPU updates here
		// GPU updates are used for highly-parallel stuffs like:
		// - Physics processing (with fluid packets, rays, densely-sampled signal processing, etc.)
		// - Planet-scale predator-prey relationships between plant/critter and critter/critter species
		//athruGPUUpdates->Update();

		// Render the area around the player
		athruRendering->Render(athruScene->GetMainCamera());

		// Record the time at this frame so we can calculate
		// [deltaTime]
		TimeStuff::timeAtLastFrame = std::chrono::steady_clock::now();
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