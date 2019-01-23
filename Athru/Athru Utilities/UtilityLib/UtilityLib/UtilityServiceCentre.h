#pragma once

#include "StackAllocator.h"
#include "leakChecker.h"
#include "Logger.h"
#include "Input.h"
#include "Application.h"
#include "AppGlobals.h"

namespace AthruCore
{
	class Utility
	{
		public:
			Utility() = delete;
			~Utility() = delete;

			static void Init(const u8Byte& expectedMemoryUsage)
			{
				// Attempt to create and register the memory-management
				// service
				stackAllocatorPttr = new StackAllocator(expectedMemoryUsage);

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

