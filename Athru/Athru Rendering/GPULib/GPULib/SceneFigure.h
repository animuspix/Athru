#pragma once

#include <directxmath.h>
#include "UtilityServiceCentre.h"

enum class FIG_TYPES
{
	PLANET,
	STAR,
	PLANT,
	CRITTER
};

class SceneFigure
{
	public:
		SceneFigure();
		SceneFigure(DirectX::XMVECTOR velo, DirectX::XMVECTOR position,
					DirectX::XMVECTOR qtnAngularVelo, DirectX::XMVECTOR qtnRotation, float scale,
					fourByteUnsigned figType, DirectX::XMVECTOR* distCoeffs, DirectX::XMVECTOR* rgbaCoeffs,
					fourByteUnsigned isNonNull);
		~SceneFigure();

		// GPU-friendly version of [this]; should only be accessed
		// indirectly through the interfaces described below
		struct Figure
		{
			Figure() {}
			Figure(DirectX::XMVECTOR velo, DirectX::XMVECTOR position,
				   DirectX::XMVECTOR qtnAngularVelo, DirectX::XMVECTOR qtnRotation, float scale,
				   fourByteUnsigned funcType, DirectX::XMVECTOR* distParams, DirectX::XMVECTOR* rgbaParams,
				   fourByteUnsigned isNonNull, address originPttr) :
				   pos(position),
				   rotationQtn(qtnRotation),
				   scaleFactor{ scale, scale, scale, 1 },
				   dfType{ funcType, 0, 0, 0 },
				   distCoeffs{ distParams[0], distParams[1], distParams[2], distParams[3], distParams[4],
							   distParams[5], distParams[6], distParams[7], distParams[8], distParams[9] },
				   rgbaCoeffs{ rgbaParams[0], rgbaParams[1], rgbaParams[2], rgbaParams[3], rgbaParams[4],
							   rgbaParams[5], rgbaParams[6], rgbaParams[7], rgbaParams[8], rgbaParams[9] },
				   nonNull{	isNonNull, 0, 0, 0 },
				   origin{ (fourByteUnsigned)(((eightByteUnsigned)originPttr & 0xFFFFFFFF00000000) >> 32),
						   (fourByteUnsigned)(((eightByteUnsigned)originPttr & 0x00000000FFFFFFFF)), 0, 0 } {}

			// The location of this figure at any particular time
			DirectX::XMVECTOR pos;

			// The quaternion rotation applied to this figure at
			// any particular time
			DirectX::XMVECTOR rotationQtn;

			// Uniform object scale
			DirectX::XMFLOAT4 scaleFactor;

			// The distance function used to render [this]
			DirectX::XMUINT4 dfType;

			// Coefficients of the distance function used to render [this]
			DirectX::XMVECTOR distCoeffs[10];

			// Coefficients of the color function used to tint [this]
			DirectX::XMVECTOR rgbaCoeffs[10];

			// Whether or not [this] has been fully defined on the GPU
			DirectX::XMUINT4 nonNull;

			// Key marking which [SceneFigure] is associated with [this]
			// (needed for tracing GPU-side [Figure]s back to CPU-side [SceneFigure]s
			// after data is read back from the graphics card)
			DirectX::XMUINT4 origin;
		};

		// Retrieve the underlying distance function used
		// to define the shape of [this]
		FIG_TYPES GetDistFuncType();

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

		// Get the coefficients of the color function associated
		// with [this]
		DirectX::XMVECTOR* GetRGBACoeffs();

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
