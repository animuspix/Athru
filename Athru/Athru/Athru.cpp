#include "HiLevelServiceCentre.h"

#include <DXGItype.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <DXProgrammableCapture.h>

void GameLoop()
{
	// Cache local references to utility services
	Application* athruApp = AthruUtilities::UtilityServiceCentre::AccessApp();
	Input* athruInput = AthruUtilities::UtilityServiceCentre::AccessInput();

	// Cache a local reference to the GPU messenger object
	GPUMessenger* athruGPUMessenger = AthruGPU::GPUServiceCentre::AccessGPUMessenger();

	// Cache a local reference to the SDF rasterizer
	FigureRaster* athruSDFRaster = AthruGPU::GPUServiceCentre::AccessRasterizer();

	// Cache a local reference to the render-manager
	Renderer* athruRendering = AthruGPU::GPUServiceCentre::AccessRenderer();

	// Cache a local reference to the GPU update manager
	//GPUUpdateManager* athruGPUUpdates = AthruGPU::GPUServiceCentre::AccessGPUUpdateManager();

	// Cache a local reference to the high-level scene representation
	Scene* athruScene = HiLevelServiceCentre::AccessScene();

	// Declare DX11 capture interface
	//#define DEBUG_FIRST_FRAME
	#ifdef DEBUG_FIRST_FRAME
		IDXGraphicsAnalysis* analysis;
		HRESULT valid = DXGIGetDebugInterface1(0, __uuidof(analysis), (void**)&analysis);
	#endif
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

		// Update engine clock (needed to evaluate delta-time/FPS)
		TimeStuff::timeAtLastFrame = std::chrono::steady_clock::now();

		// Log frame count
		//AthruUtilities::UtilityServiceCentre::AccessLogger()->Log(TimeStuff::frameCtr);

		// Update the game
		athruScene->Update();

		// Optionally begin debug capture
		#ifdef DEBUG_FIRST_FRAME
			if (TimeStuff::frameCtr == 0)
			{
				analysis->BeginCapture();
			}
		#endif

		// Pass scene data to the GPU for updates/rendering
		athruGPUMessenger->SceneToGPU(AthruGPU::GPUServiceCentre::AccessD3D()->GetDeviceContext(),
									  athruScene->CollectLocalFigures());

		// Pass the SDF raster-atlas along to the GPU each frame
		athruSDFRaster->RasterToGPU(AthruGPU::GPUServiceCentre::AccessD3D()->GetDeviceContext());

		// Rasterize planets on the zeroth frame + whenever the player hits a new system
		if (athruScene->CheckFreshSys() || TimeStuff::frameCtr == 0)
		{
			athruSDFRaster->RasterPlanets(AthruGPU::GPUServiceCentre::AccessD3D()->GetDeviceContext());
		}

		// Perform GPU updates here
		// GPU updates are used for highly-parallel stuffs like:
		// - Physics processing (with fluid packets, rays, densely-sampled signal processing, etc.)
		// - Planet-scale predator-prey relationships between plant/critter and critter/critter species
		//athruGPUUpdates->Update();

		// Render the area around the player
		athruRendering->Render(athruScene->GetMainCamera());

		// End the debug capture started above
		#ifdef DEBUG_FIRST_FRAME
			if (TimeStuff::frameCtr == 0)
			{
				analysis->EndCapture();
				analysis->Release();
			}
		#endif

		// Update the frame counter
		TimeStuff::frameCtr += 1;
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