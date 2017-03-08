#pragma once

#include "DirectionalLight.h"
#include "Planet.h"

class StarSystem
{
	public:
		StarSystem();
		~StarSystem();

	private:
		// The star associated with this system
		DirectionalLight sun;

		// The planets associated with this system
		Planet* planets;
};

