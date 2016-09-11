#pragma once

#include "Map.h"
#include "Service.h"

#include "leakChecker.h"

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

		// Register a new/separate service
		/*static*/ void Register(Service* service);

		// Fetch a given service by name
		/*static*/ Service* Fetch(char* serviceName);

	private:
		// Dictionary of available services
		/*static*/ Map<char*, Service*>* serviceBox;

		// Private constructor/destructor
		ServiceCentre();
		~ServiceCentre();

		// Private singleton instance of [this]
		static ServiceCentre* self;

		// Note that this approach requires an expensive character
		// check EVERY TIME a service is requested, which could
		// significantly impact performance; if performance is
		// especially affected, consider abandoning array-style
		// storage and returning to the less-elegant-but-faster
		// approach used by Nystrom in Game Programming Patterns
};

