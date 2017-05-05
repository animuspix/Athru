#pragma once

// High-level Athru engine classes
#include "SceneManager.h"
#include "Graphics.h"

// Athru rendering systems
#include "Direct3D.h"
#include "TextureManager.h"
#include "RenderManager.h"
#include "NormalBuilder.h"
#include "PlanarUnwrapper.h"

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
			// Assume 2GB memory usage for now; update with better
			// values when possible
			eightByteUnsigned STARTING_HEAP = INT_MAX;
			AthruUtilities::UtilityServiceCentre::Init(STARTING_HEAP);

			// Attempt to create and register the primary graphics
			// service
			graphicsPttr = new Graphics(AthruUtilities::UtilityServiceCentre::AccessApp()->GetHWND());

			// Attempt to create and register the texture manager
			Direct3D* d3DPttr = graphicsPttr->GetD3D();
			textureManagerPttr = new TextureManager(d3DPttr->GetDevice());

			// Attempt to create and register the render manager
			// Getting the post-processing texture is kinda hacky here;
			// startup process/architecture should probably be adjusted so
			// that [Direct3D] is a service in its own right and not a
			// member of [Graphics]
			renderManagerPttr = new RenderManager(d3DPttr->GetDeviceContext(), d3DPttr->GetDevice(),
												  AVAILABLE_POST_EFFECTS::BLOOM, AVAILABLE_POST_EFFECTS::DEPTH_OF_FIELD,
												  textureManagerPttr->GetInternalTexture2D(AVAILABLE_INTERNAL_TEXTURES::SCREEN_TEXTURE).asShaderResource);

			// Attempt to create and register the scene manager
			sceneManagerPttr = new SceneManager();

			// Now that the Draw Manager and the Scene Manager exist,
			// we can initialise their references within the Graphics class;
			// do that here
			graphicsPttr->FetchDependencies();
		}

		static void ShutDown()
		{
			// Free any un-managed memory allocated to utility services; 
			// also send the references stored for each utility to [nullptr]
			AthruUtilities::UtilityServiceCentre::DeInitApp();
			AthruUtilities::UtilityServiceCentre::DeInitInput();
			AthruUtilities::UtilityServiceCentre::DeInitLogger();

			// Free any un-managed memory allocated to higher-level services,
			// then send the references stored for each service to [nullptr]
			sceneManagerPttr->~SceneManager();
			renderManagerPttr->~RenderManager();
			textureManagerPttr->~TextureManager();
			graphicsPttr->~Graphics();

			graphicsPttr = nullptr;
			textureManagerPttr = nullptr;
			renderManagerPttr = nullptr;
			sceneManagerPttr = nullptr;

			// Free memory associated with the [StackAllocator] (all managed memory + 
			// anything required by the [StackAllocator] itself) and send the reference
			// associated with it to [nullptr]
			AthruUtilities::UtilityServiceCentre::DeInitMemory();
		}

		static Graphics* AccessGraphics()
		{
			return graphicsPttr;
		}

		static TextureManager* AccessTextureManager()
		{
			return textureManagerPttr;
		}

		static RenderManager* AccessRenderManager()
		{
			return renderManagerPttr;
		}

		static SceneManager* AccessSceneManager()
		{
			return sceneManagerPttr;
		}

	private:
		// Pointers to available services
		static Graphics* graphicsPttr;
		static TextureManager* textureManagerPttr;
		static RenderManager* renderManagerPttr;
		static SceneManager* sceneManagerPttr;
};