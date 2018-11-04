#pragma once

#include <directxmath.h>
#include "UtilityServiceCentre.h"

class SceneFigure
{
	public:
		SceneFigure();
		SceneFigure(DirectX::XMFLOAT3 position, float scale,
					DirectX::XMVECTOR* distCoeffs);
		~SceneFigure();

		// GPU-friendly version of [this]; should only be accessed
		// indirectly through the interfaces described below
		struct Figure
		{
			Figure() {}
			Figure(DirectX::XMFLOAT4 linTrans,
				   DirectX::XMVECTOR* distParams) :
				   linTransf(linTrans),
				   distCoeffs{ distParams[0], distParams[1], distParams[2] } {}
			// The location of this figure at any particular time
			// (xyz) and a uniform scale factor (w)
			DirectX::XMFLOAT4 linTransf;

			// Coefficients of the distance function used to render [this]
			DirectX::XMVECTOR distCoeffs[3];
		};

		// Get a copy of the GPU-friendly [Figure] associated with [this]
		Figure GetCoreFigure();

		// Replace the [Figure] associated with [this] with one provided
		// externally (i.e. one from the GPU)
		void SetCoreFigure(Figure& fig);

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void* operator new[](size_t size);
		void operator delete(void* target);
		void operator delete[](void* target);

	protected:
		// The actual data payload that travels to the GPU each frame
		Figure coreFigure;
};
