#pragma once

#include "StackAllocator.h"
#include "Logger.h"
#include "Input.h"
#include "Application.h"
#include "Direct3D.h"
#include "RenderManager.h"
#include "Graphics.h"
#include "SceneManager.h"
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

			// Attempt to create and register the render manager
			Direct3D* d3DPttr = graphicsPttr->GetD3D();
			renderManagerPttr = new RenderManager(d3DPttr->GetDeviceContext(), d3DPttr->GetDevice(), d3DPttr->GetViewport(),
												  AVAILABLE_POST_EFFECTS::BLOOM, AVAILABLE_POST_EFFECTS::DEPTH_OF_FIELD, AVAILABLE_POST_EFFECTS::NULL_EFFECT,
												  AVAILABLE_LIGHTING_SHADERS::COOK_TORRANCE_PBR);

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
			graphicsPttr->~Graphics();
			appPttr->~Application();
			inputPttr->~Input();
			loggerPttr->~Logger();

			loggerPttr = nullptr;
			inputPttr = nullptr;
			appPttr = nullptr;
			graphicsPttr = nullptr;
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
		static RenderManager* renderManagerPttr;
		static SceneManager* sceneManagerPttr;
};