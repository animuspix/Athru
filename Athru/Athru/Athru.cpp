#include "HiLevelServiceCentre.h"

#include <DXGItype.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <DXProgrammableCapture.h>

void GameLoop()
{
	// Cache local references to utility services
	Application* athruApp = AthruCore::Utility::AccessApp();
	Input* athruInput = AthruCore::Utility::AccessInput();

	// Cache a local reference to the GPU messenger object
	GPUMessenger* athruGPUMessenger = AthruGPU::GPU::AccessGPUMessenger();

	// Cache a local reference to the SDF rasterizer
	//GradientMapper* athruSDFRaster = AthruGPU::GPU::AccessRasterizer();

	// Cache a local reference to the render-manager
	Renderer* athruRendering = AthruGPU::GPU::AccessRenderer();

	// Cache a local reference to the GPU update manager
	//GPUUpdateManager* athruGPUUpdates = AthruGPU::GPUServiceCentre::AccessGPUUpdateManager();

	// Cache a local reference to the high-level scene representation
	Scene* athruScene = HiLevelServiceCentre::AccessScene();

    // Cache a local reference to the Direct3D handler object
    Direct3D* d3d = AthruGPU::GPU::AccessD3D();

    // Cache a local reference to the Direct3D command list

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
		TimeStuff::lastFrameTime = std::chrono::steady_clock::now();

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

		// Upload system data, rasterize gradient volumes on the zeroth frame + whenever the player hits a new system
		bool sysLoaded = false;
		if (/*athruScene->CheckFreshSys() || */TimeStuff::frameCtr == 0)
		{
			//athruSDFRaster->RasterPlanets(AthruGPU::GPU::AccessD3D()->GetDeviceContext());
			sysLoaded = true;
            athruGPUMessenger->SysToGPU(athruScene->CollectLocalFigures());
		}

		// Pass the scene to the gpu whenever planets aren't being rasterized
		if (!sysLoaded)
		{
			// Pass the read-only SDF raster-atlas along to the GPU every frame
			//athruSDFRaster->RasterToGPU(AthruGPU::GPU::AccessD3D()->GetDeviceContext());

			// Pass generic input data along to the GPU
			//DirectX::XMVECTOR v = athruScene->GetGalaxy()->GetCurrentSystem(athruScene->GetMainCamera()->GetTranslation())->GetPos();
			//DirectX::XMFLOAT4 p =;
			//DirectX::XMStoreFloat4(p, v);
			athruGPUMessenger->InputsToGPU(DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f),
                                           athruScene->GetMainCamera()); // Focussing on rendering for now, avoiding directly
																	     // working with system positions until I need to

			// Perform GPU updates here
			// GPU updates are used for highly-parallel stuffs like:
			// - Physics processing (with fluid packets, rays, densely-sampled signal processing, etc.)
			// - Planet-scale predator-prey relationships between plant/critter and critter/critter species
			//athruGPUUpdates->Update();

			// Render the scene
			athruRendering->Render(d3d);
		}

		// End the debug capture started above
		#ifdef DEBUG_FIRST_FRAME
			if (TimeStuff::frameCtr == 0)
			{
				analysis->EndCapture();
				analysis->Release();
			}
		#endif

		// FPS reading/logging
		// Small penalty from output, but useful for performance measurement when graphics debugging isn't available
		// (like e.g. on school/work PCs without admin privileges)
		//AthruCore::Utility::AccessLogger()->Log(TimeStuff::FPS(), "FPS");
		AthruCore::Utility::AccessLogger()->Log<Logger::DESTINATIONS::LOG_FILE>(TimeStuff::deltaTime() * 1000.0f, "Time between frames (milliseconds)");
		//AthruCore::Utility::AccessLogger()->Log<Logger::DESTINATIONS::LOG_FILE>(TimeStuff::frameCtr, "Frame counter");

		// Update the frame counter
		TimeStuff::frameCtr += 1;
	}

	// Quick memory occupancy profile for tuning
	AthruCore::Utility::AccessLogger()->LogMem(AthruCore::Utility::AccessMemory()->GetStart());
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