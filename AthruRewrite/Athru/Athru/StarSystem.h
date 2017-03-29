#pragma once

#include "Planet.h"

class StarSystem
{
	public:
		StarSystem();
		~StarSystem();

	private:
		// The star associated with this system
		Boxecule sun;

		// The planets associated with this system
		Planet* planets;
};

