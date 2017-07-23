#pragma once

#include <directxmath.h>
#include "Typedefs.h"
#include "SceneFigure.h"

// Mineral-based planet colors are probably best, but minerals definitions are a whole
// separate area that I haven't really thought about yet; just use single colors per-planet
// for now

class Planet : public SceneFigure
{
	public:
		Planet(float givenMass, float givenRadius, DirectX::XMFLOAT4 givenAvgColor,
			   DirectX::XMVECTOR offsetFromStar, DirectX::XMFLOAT3 rotation);
		~Planet();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);
};
