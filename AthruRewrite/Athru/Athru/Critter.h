#pragma once

#include "SceneFigure.h"

class Critter : public SceneFigure
{
	public:
		Critter();
		~Critter();

		// Overload the standard allocation/de-allocation operators
		//void* operator new(size_t size);
		//void operator delete(void* target);

	private:
		// Critters are defined with IFS fractal systems;
		// color, animation, and physical behaviour are defined
		// procedurally according to fractal properties + interactions
		// between individual Critters and their local environment
};

