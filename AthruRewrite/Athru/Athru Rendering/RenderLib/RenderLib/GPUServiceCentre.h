#pragma once

#include "Direct3D.h"
#include "TextureManager.h"
#include "RenderManager.h"
#include "PlanarUnwrapper.h"
#include "GPURand.h"
#include "UtilityServiceCentre.h"

namespace AthruGPU
{
	class GPUServiceCentre
	{
	public:
		GPUServiceCentre() = delete;
		~GPUServiceCentre() = delete;

		static void Init()
		{
			// Initialise utilities (if neccessary)
			if (AthruUtilities::UtilityServiceCentre::AccessMemory() == nullptr)
			{
				// Allocation assumes Athru will use 255 megabytes at most
				const eightByteUnsigned STARTING_HEAP = 255000000;
				AthruUtilities::UtilityServiceCentre::Init(STARTING_HEAP);
				internalInit = true;
			}

			else
			{
				internalInit = false;
			}

			// Initialise the Direct3D handler class, the texture manager, and the rendering manager
			d3DPttr = DEBUG_NEW Direct3D(AthruUtilities::UtilityServiceCentre::AccessApp()->GetHWND());
			textureManagerPttr = new TextureManager(d3DPttr->GetDevice());
			renderManagerPttr = new RenderManager(textureManagerPttr->GetDisplayTexture(AVAILABLE_DISPLAY_TEXTURES::SCREEN_TEXTURE).asReadOnlyShaderResource);

			// Initialise the GPU-side random number generator
			gpuRand = new GPURand(d3DPttr->GetDevice());
		}

		static void DeInit()
		{
			// Clean-up the texture manager + the rendering manager
			textureManagerPttr->~TextureManager();
			textureManagerPttr = nullptr;

			renderManagerPttr->~RenderManager();
			renderManagerPttr = nullptr;

			// Clean-up data associated with the GPU-side random
			// number generator
			gpuRand->gpuRandState->Release();
			gpuRand->gpuRandState = nullptr;
			gpuRand->gpuRandStateView->Release();
			gpuRand->gpuRandStateView = nullptr;
			gpuRand = nullptr;

			// Clean-up the Direct3D handler class
			delete d3DPttr;
			d3DPttr = nullptr;

			if (internalInit)
			{
				// Free any un-managed memory allocated to utility services;
				// also send the references stored for each utility to [nullptr]
				AthruUtilities::UtilityServiceCentre::DeInitApp();
				AthruUtilities::UtilityServiceCentre::DeInitInput();
				AthruUtilities::UtilityServiceCentre::DeInitLogger();
			}
		}

		static Direct3D* AccessD3D()
		{
			return d3DPttr;
		}

		static TextureManager* AccessTextureManager()
		{
			return textureManagerPttr;
		}

		static RenderManager* AccessRenderManager()
		{
			return renderManagerPttr;
		}

		static ID3D11UnorderedAccessView* AccessGPURandView()
		{
			return gpuRand->gpuRandStateView;
		}

	private:
		// General GPU interfacing
		static Direct3D* d3DPttr;

		// 2D and 3D texture management
		static TextureManager* textureManagerPttr;

		// Visibility/lighting calculations, also
		// post-production and presentation
		static RenderManager* renderManagerPttr;

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
}
