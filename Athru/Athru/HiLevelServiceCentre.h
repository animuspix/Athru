#pragma once

// High-level Athru engine classes
#include "Scene.h"

// Athru rendering classes
#include "GPUServiceCentre.h"

// Athru utility classes
#include "UtilityServiceCentre.h"

class HiLevelServiceCentre
{
	public:
		HiLevelServiceCentre() = delete;
		~HiLevelServiceCentre() = delete;

		static void StartUp()
		{
			// Allocation assumes Athru will use 255 megabytes at most
			const eightByteUnsigned STARTING_HEAP = 255000000;
			AthruUtilities::UtilityServiceCentre::Init(STARTING_HEAP);

			// Attemp to create and register rendering services
			// (the render-manager + the texture-manager)
			AthruGPU::GPUServiceCentre::Init();

			// Attempt to create and register the scene service
			scenePttr = new Scene();
		}

		static void ShutDown()
		{
			// Free any un-managed memory allocated to higher-level services,
			// then send the references stored for each service to [nullptr]
			scenePttr->~Scene();
			scenePttr = nullptr;

			// Free any un-managed memory allocated to rendering services;
			// also send the references stored for each rendering service to
			// [nullptr]
			AthruGPU::GPUServiceCentre::DeInit();

			// Free any un-managed memory allocated to utility services;
			// also send the references stored for each utility to [nullptr]
			AthruUtilities::UtilityServiceCentre::DeInitApp();
			AthruUtilities::UtilityServiceCentre::DeInitInput();
			AthruUtilities::UtilityServiceCentre::DeInitLogger();

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