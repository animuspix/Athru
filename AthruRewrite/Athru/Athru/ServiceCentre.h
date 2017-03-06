#pragma once

#include "StackAllocator.h"
#include "Logger.h"
#include "Input.h"
#include "Application.h"
#include "Direct3D.h"
#include "Graphics.h"
#include "AthruGlobals.h"

class ServiceCentre
{
	public:
		ServiceCentre() = delete;
		~ServiceCentre() = delete;

		static void StartUp()
		{
			// Attempt to create and register the memory-management
			// service, then abandon ship if creation fails
			stackAllocatorPtr = new StackAllocator();

			// Attempt to create and register the logging
			// service
			loggerPtr = new Logger("log.txt");

			// Attempt to create and register the primary input service
			inputPtr = new Input();

			// Create and register graphics support services here

			// Attempt to create and register the application
			appPtr = new Application();

			// Attempt to create and register the primary graphics
			// service
			graphicsPtr = new Graphics(appPtr->GetHWND(), loggerPtr);

			// Create and register additional application-support
			// services here

			// Create and register additional, independent services over here

			// Place a marker to separate long-term, app-duration
			// memory from short-term memory that will be written and over-written on
			// the fly
			stackAllocatorPtr->SetMarker();
		}

		static void ShutDown()
		{
			graphicsPtr->~Graphics();
			appPtr->~Application();
			inputPtr->~Input();
			loggerPtr->~Logger();

			loggerPtr = nullptr;
			inputPtr = nullptr;
			graphicsPtr = nullptr;
			appPtr = nullptr;

			delete stackAllocatorPtr;
			stackAllocatorPtr = nullptr;
		}

		static StackAllocator* AccessMemory()
		{
			return stackAllocatorPtr;
		}

		static Logger* AccessLogger()
		{
			return loggerPtr;
		}

		static Input* AccessInput()
		{
			return inputPtr;
		}

		static Application* AccessApp()
		{
			return appPtr;
		}

		static Graphics* AccessGraphics()
		{
			return graphicsPtr;
		}

	private:
		// Pointers to available services
		static StackAllocator* stackAllocatorPtr;
		static Logger* loggerPtr;
		static Input* inputPtr;
		static Application* appPtr;
		static Graphics* graphicsPtr;
};