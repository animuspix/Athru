#pragma once

#include "Direct3D.h"
#include "GPUMemory.h"
#include "Renderer.h"
#include "GPUUpdateManager.h"
#include "GPUMessenger.h"
#include "FigureRaster.h"
#include "GPURand.h"
#include "PlanarUnwrapper.h"
#include "UtilityServiceCentre.h"
#include "GPUGlobals.h"

namespace AthruGPU
{
	class GPU
	{
	public:
		GPU() = delete;
		~GPU() = delete;

		static void Init()
		{
			// Initialise utilities (if neccessary)
			if (AthruCore::Utility::AccessMemory() == nullptr)
			{
				// Allocation assumes Athru will use 255 megabytes at most
				const u8Byte STARTING_HEAP = 255000000;
				AthruCore::Utility::Init(STARTING_HEAP);
				internalInit = true;
			}
			else
			{
				internalInit = false;
			}

			// Initialise the Direct3D handler class, the GPU messenger, the GPU-side random number generator,
			// and the dispatch argument trimmers
			d3DPttr = DEBUG_NEW Direct3D(AthruCore::Utility::AccessApp()->GetHWND());
			gpuMessengerPttr = new GPUMessenger(d3DPttr->GetDevice());
			gpuRand = new GPURand(d3DPttr->GetDevice());

			// Initialize the SDF rasterizer
			HWND winHandle = AthruCore::Utility::AccessApp()->GetHWND();
			rasterPttr = new FigureRaster(d3DPttr->GetDevice(), winHandle);

			// Initialise the rendering manager + the GPU update manager
			rendererPttr = new Renderer(winHandle,
										d3DPttr->GetDevice(),
										d3DPttr->GetDeviceContext());

			//gpuUpdateManagerPttr = new GPUUpdateManager();
		}

		static void DeInit()
		{
			// Clean-up the rendering manager
			rendererPttr->~Renderer();
			rendererPttr = nullptr;

			// Clean-up the GPU update manager
			//gpuUpdateManagerPttr->~GPUUpdateManager();
			//gpuUpdateManagerPttr = nullptr;

			// Clean-up data associated with the GPU
			// random-number generator
			gpuRand->~GPURand();

			// Clean-up data associated with the SDF rasterizer
			rasterPttr->~FigureRaster();

			// Clean-up data associated with the GPU messenger
			gpuMessengerPttr->~GPUMessenger();
			gpuMessengerPttr = nullptr;

			// Clean-up the Direct3D handler class
			delete d3DPttr;
			d3DPttr = nullptr;

			if (internalInit)
			{
				// Free any un-managed memory allocated to utility services;
				// also send the references stored for each utility to [nullptr]
				AthruCore::Utility::DeInitApp();
				AthruCore::Utility::DeInitInput();
				AthruCore::Utility::DeInitLogger();
			}
		}

		static Direct3D* AccessD3D()
		{
			return d3DPttr;
		}

		static GPUMessenger* AccessGPUMessenger()
		{
			return gpuMessengerPttr;
		}

		static FigureRaster* AccessRasterizer()
		{
			return rasterPttr;
		}

		static Renderer* AccessRenderer()
		{
			return rendererPttr;
		}

		//static GPUUpdateManager* AccessGPUUpdateManager()
		//{
		//	return gpuUpdateManagerPttr;
		//}

		static const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& AccessGPURandView()
		{
			return gpuRand->gpuRandState.view();
		}

	private:
		// General GPU interfacing
		static Direct3D* d3DPttr;

		// GPU figure sync, maintains equivalence between
		// CPU and GPU figure data
		static GPUMessenger* gpuMessengerPttr;

		// Volume rasterization object, used for preparing SDF textures when players travel between systems
		// (planet rasterization) or planets (animal/plant rasterization)
		static FigureRaster* rasterPttr;

		// Visibility/lighting calculations, also
		// post-production and presentation
		static Renderer* rendererPttr;

		// GPU updates for highly-parallel data
		// (animal cells, plant/animal predation,
		// physics processing, etc)
		//static GPUUpdateManager* gpuUpdateManagerPttr;

		// Simple switch describing the application
		// associated with [this] was initialized
		// externally (in a separate high-level
		// service centre) or internally (immediately
		// before the Direct3D handler + the texture
		// manager + the render manager were created)
		static bool internalInit;

		// Wrapper around a state buffer designed for
		// improved GPU random number generation
		static GPURand* gpuRand;
	};
};