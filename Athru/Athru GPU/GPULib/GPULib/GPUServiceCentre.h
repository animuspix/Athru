#pragma once

#include "Direct3D.h"
#include "GPUMemory.h"
#include "Renderer.h"
#include "GPUMessenger.h"
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
				AthruCore::Utility::Init(MemoryStuff::STARTING_HEAP_ALLOC);
				internalInit = true;
			}
			else
			{
				internalInit = false;
			}

			// Initialise the Direct3D handler class, the GPU messenger, the GPU-side random number generator,
			// and the dispatch argument trimmers
			d3DPttr = DEBUG_NEW Direct3D(AthruCore::Utility::AccessApp()->GetHWND());
			GPUMemory& gpuMem = d3DPttr->GetGPUMem();
            const Microsoft::WRL::ComPtr<ID3D12Device>& device = d3DPttr->GetDevice();
			gpuMessengerPttr = new GPUMessenger(device, gpuMem);

			// Initialize the SDF rasterizer
			HWND winHandle = AthruCore::Utility::AccessApp()->GetHWND();
			//rasterPttr = new GradientMapper(device, gpuMem, winHandle);

			// Initialise the rendering manager + the GPU update manager
			rendererPttr = new Renderer(winHandle, gpuMem,
										device, d3DPttr->GetGraphicsQueue());
		}

		static void DeInit()
		{
			// Clean-up the rendering manager
			rendererPttr->~Renderer();
			rendererPttr = nullptr;

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

		static Renderer* AccessRenderer()
		{
			return rendererPttr;
		}
	private:
		// General GPU interfacing
		static Direct3D* d3DPttr;

		// GPU figure sync, maintains equivalence between
		// CPU and GPU figure data
		static GPUMessenger* gpuMessengerPttr;

		// Visibility/lighting calculations, also
		// post-production and presentation
		static Renderer* rendererPttr;

		// Simple switch describing the application
		// associated with [this] was initialized
		// externally (in a separate high-level
		// service centre) or internally (immediately
		// before the Direct3D handler + the texture
		// manager + the render manager were created)
		static bool internalInit;
	};
};