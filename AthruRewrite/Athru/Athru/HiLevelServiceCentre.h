#pragma once

// High-level Athru engine classes
#include "Scene.h"

// Athru rendering classes
#include "RenderServiceCentre.h"

// Athru utility classes
#include "UtilityServiceCentre.h"

class HiLevelServiceCentre
{
	public:
		HiLevelServiceCentre() = delete;
		~HiLevelServiceCentre() = delete;

		static void StartUp()
		{
			// Attempt to create and register utility services
			// (memory, logging, input, the application)
			// Assume 4GB memory usage for now; update with better
			// values when possible
			eightByteUnsigned STARTING_HEAP = UINT_MAX;
			AthruUtilities::UtilityServiceCentre::Init(STARTING_HEAP);

			// Attemp to create and register rendering services
			// (the render-manager + the texture-manager)
			AthruRendering::RenderServiceCentre::Init();

			// Attempt to create and register the scene service
			scenePttr = new Scene();
		}

		static void ShutDown()
		{
			// Free any un-managed memory allocated to rendering services;
			// also send the references stored for each rendering service to
			// [nullptr]
			AthruRendering::RenderServiceCentre::DeInit();

			// Free any un-managed memory allocated to utility services;
			// also send the references stored for each utility to [nullptr]
			AthruUtilities::UtilityServiceCentre::DeInitApp();
			AthruUtilities::UtilityServiceCentre::DeInitInput();
			AthruUtilities::UtilityServiceCentre::DeInitLogger();

			// Free any un-managed memory allocated to higher-level services,
			// then send the references stored for each service to [nullptr]
			scenePttr->~Scene();
			scenePttr = nullptr;

			// Free memory associated with the [StackAllocator] (all managed memory +
			// anything required by the [StackAllocator] itself) and send the reference
			// associated with it to [nullptr]
			AthruUtilities::UtilityServiceCentre::DeInitMemory();
		}

		static Scene* AccessScene()
		{
			return scenePttr;
		}

	private:
		// Pointers to available high-level services
		static Scene* scenePttr;
};