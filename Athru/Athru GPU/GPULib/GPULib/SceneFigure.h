#pragma once

#include <directxmath.h>
#include "UtilityServiceCentre.h"

class SceneFigure
{
	public:
		SceneFigure();
		SceneFigure(DirectX::XMFLOAT3 position,
					DirectX::XMVECTOR qtnRotation, float scale,
					DirectX::XMVECTOR* distCoeffs);
		~SceneFigure();

		// GPU-friendly version of [this]; should only be accessed
		// indirectly through the interfaces described below
		struct Figure
		{
			Figure() {}
			Figure(DirectX::XMFLOAT4 linTrans,
				   DirectX::XMVECTOR qtnRotation,
				   DirectX::XMVECTOR* distParams) :
				   linTransf(linTrans),
				   rotationQtn(qtnRotation),
				   distCoeffs{ distParams[0], distParams[1], distParams[2] } {}
			// The location of this figure at any particular time
			// (xyz) and a uniform scale factor (w)
			DirectX::XMFLOAT4 linTransf;

			// The quaternion rotation applied to this figure at
			// any particular time
			DirectX::XMVECTOR rotationQtn;

			// Coefficients of the distance function used to render [this]
			DirectX::XMVECTOR distCoeffs[3];
		};

		// Rotate [this] by the given angle "around" the
		// given vector
		void SetRotation(DirectX::XMVECTOR axis,
						 float angle);

		// Get the quaternion rotation associated with [this]
		// To convert to axis-angle:
		// - Extract the half-angle by taking the arccos of [w]
		// - Extract the axis by dividing the vector part by the
		//   sine of the half-angle (try to avoid division by zero
		//   here)
		// - Extract the angle by doubling the half-angle (duh)
		DirectX::XMVECTOR GetQtnRotation();

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
