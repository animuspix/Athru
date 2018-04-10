#pragma once

#include <directxmath.h>
#include "System.h"

enum class AVAILABLE_GALACTIC_LAYOUTS
{
	SPHERE,
	SPIRAL,
	NULL_LAYOUT
};

class Galaxy
{
	public:
		Galaxy(AVAILABLE_GALACTIC_LAYOUTS galacticLayout);
		~Galaxy();

		// Return an array carrying all the star systems in the game
		System** GetSystems();

		// Find the system closest to the given camera position
		System* GetCurrentSystem(DirectX::XMVECTOR& cameraPos);

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		System* systems[SceneStuff::SYSTEM_COUNT];
};

