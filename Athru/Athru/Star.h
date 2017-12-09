#pragma once

#include <directxmath.h>
#include "SceneFigure.h"

class Star : public SceneFigure
{
	public:
		Star() {}
		Star(float givenRadius,
			 DirectX::XMVECTOR position,
			 DirectX::XMVECTOR* distCoeffs, DirectX::XMVECTOR* rgbaCoeffs);
		~Star();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);
};
