#pragma once

#include "StackAllocator.h"
#include "Logger.h"
#include "Input.h"
#include "Application.h"
#include "AthruGlobals.h"

namespace AthruUtilities
{
	class UtilityServiceCentre
	{
		public:
			UtilityServiceCentre() = delete;
			~UtilityServiceCentre() = delete;

			static void Init()
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
			}

			static void DeInitMemory()
			{
				delete stackAllocatorPttr;
				stackAllocatorPttr = nullptr;
			}

			static void DeInitLogger()
			{
				loggerPttr->~Logger();
				loggerPttr = nullptr;
			}

			static void DeInitInput()
			{
				inputPttr->~Input();
				inputPttr = nullptr;
			}

			static void DeInitApp()
			{
				appPttr->~Application();
				appPttr = nullptr;
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

		private:
			static StackAllocator* stackAllocatorPttr;
			static Logger* loggerPttr;
			static Input* inputPttr;
			static Application* appPttr;
	};
}

