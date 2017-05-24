#pragma once

#include "Star.h"
#include "Planet.h"

class System
{
	public:
		System();
		~System();

		// Update the orbits of the items associated with [this]
		// (just planet spins/orbits for now, but if star color
		// transitions were implemented this is where they'd
		// happen)
		void Update();

		// Fetch a write-allowed reference to the global
		// position of [this]
		DirectX::XMVECTOR& FetchPos();

		// Retrieve a reference to the star associated with [this]
		Star* GetStar();

		// Retrieve a reference to the planets associated with [this]
		Planet** GetPlanets();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void* operator new[](size_t size);
		void operator delete(void* target);
		void operator delete[](void* target);

	private:
		Star* star;
		Planet* planets[3];
		DirectX::XMVECTOR position;
};

