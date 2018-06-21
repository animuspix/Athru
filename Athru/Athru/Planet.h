#pragma once

#include <directxmath.h>
#include "Typedefs.h"
#include "SceneFigure.h"

class Planet : public SceneFigure
{
	public:
		Planet(float givenScale,
			   DirectX::XMFLOAT3 position, DirectX::XMVECTOR qtnRotation,
			   DirectX::XMVECTOR* distCoeffs);
		~Planet();

		// Retrieve a write-allowed reference to the specified critter
		SceneFigure& FetchCritter(fourByteUnsigned ndx);

		// Retrieve a write-allowed reference to the specified vegetation
		SceneFigure& FetchPlant(fourByteUnsigned ndx);

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		SceneFigure* critters;
		SceneFigure* plants;
};
