#pragma once

#include "StackAllocator.h"
#include "Logger.h"
#include "Input.h"
#include "Application.h"
#include "Direct3D.h"
#include "TextureManager.h"
#include "RenderManager.h"
#include "Graphics.h"
#include "SceneManager.h"
#include "PlanarUnwrapper.h"
#include "NormalBuilder.h"
#include "AthruGlobals.h"

class ServiceCentre
{
	public:
		ServiceCentre() = delete;
		~ServiceCentre() = delete;

		static void StartUp()
		{
			// Attempt to create and register the memory-management
			// service
			stackAllocatorPttr = new StackAllocator();

			// Attempt to create and register the logging
			// service
			loggerPttr = new Logger("log.txt");

			// Attempt to create and register the primary input service
			inputPttr = new Input();

			// Attempt to create and register the application
			appPttr = new Application();

			// Attempt to create and register the primary graphics
			// service
			graphicsPttr = new Graphics(appPttr->GetHWND());

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

			// Now that the Render Manager and the Scene Manager exist,
			// we can initialise their references within the Graphics class;
			// do that here
			graphicsPttr->FetchDependencies();
		}

		static void ShutDown()
		{
			sceneManagerPttr->~SceneManager();
			renderManagerPttr->~RenderManager();
			textureManagerPttr->~TextureManager();
			graphicsPttr->~Graphics();
			appPttr->~Application();
			inputPttr->~Input();
			loggerPttr->~Logger();

			loggerPttr = nullptr;
			inputPttr = nullptr;
			appPttr = nullptr;
			graphicsPttr = nullptr;
			textureManagerPttr = nullptr;
			renderManagerPttr = nullptr;
			sceneManagerPttr = nullptr;

			delete stackAllocatorPttr;
			stackAllocatorPttr = nullptr;
		}

		static StackAllocator* AccessMemory()
		{
			return stackAllocatorPttr;
		}

		static Logger* AccessLogger()
		{
			return loggerPttr;
		}

		static Input* AccessInput()
		{
			return inputPttr;
		}

		static Application* AccessApp()
		{
			return appPttr;
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
		static StackAllocator* stackAllocatorPttr;
		static Logger* loggerPttr;
		static Input* inputPttr;
		static Application* appPttr;
		static Graphics* graphicsPttr;
		static TextureManager* textureManagerPttr;
		static RenderManager* renderManagerPttr;
		static SceneManager* sceneManagerPttr;
};