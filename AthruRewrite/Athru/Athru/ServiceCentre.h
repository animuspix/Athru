#pragma once

#include "StackAllocator.h"
#include "Logger.h"
#include "Input.h"
#include "Application.h"
#include "Graphics.h"

class ServiceCentre
{
	public:
		// Function to allocate the instance (constructor wrapper)
		static void Create()
		{
			self = new ServiceCentre();
		}

		// Function to fetch the instance
		static ServiceCentre& Instance()
		{
			return *self;
		}

		// Function to de-allocate the instance (destructor wrapper)
		static void Destroy()
		{
			delete self;
			self = nullptr;
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

		// Private constructor/destructor
		ServiceCentre();
		~ServiceCentre();

		// Private clean-up function, deconstructs
		// services during ServiceCentre destruction
		void CleanUp();

		// Private singleton instance of [this]
		static ServiceCentre* self;
};