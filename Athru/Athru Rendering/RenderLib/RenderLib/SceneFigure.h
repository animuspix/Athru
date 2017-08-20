#pragma once

#include <directxmath.h>
#include "UtilityServiceCentre.h"

// All planets are julia fractals with different constants
// blended with a spherical isosurface; all grasses are
// distorted rays; most plants + all animals are IFS
// fractals (stars are coloured spheres with a blur attached)
// Requires a standard notation if we want to keep using a
// single GPU messenger class
// Potentially sensible to have an generic figure enum, then
// a carrier variable holding a ten-item array of coefficients
// that can simultaneously represent julia constants, grass
// circumferences + bezier constants, and IFS parameters
enum class FIG_TYPES
{
	STAR,
	PLANET,
	GRASS,
	IFS
};

class SceneFigure
{
	public:
		SceneFigure();
		SceneFigure(DirectX::XMVECTOR velo, DirectX::XMVECTOR position,
				    DirectX::XMVECTOR qtnAngularVelo, DirectX::XMVECTOR qtnRotation, float scale,
					DirectX::XMVECTOR surfPalette, FIG_TYPES figType);
		~SceneFigure();

		// GPU-friendly version of [this]; should only be accessed
		// indirectly through the interfaces described below
		struct Figure
		{
			Figure() {}
			Figure(DirectX::XMVECTOR velo, DirectX::XMVECTOR position,
				   DirectX::XMVECTOR qtnAngularvelo, DirectX::XMVECTOR qtnRotation, float scale,
				   fourByteUnsigned funcType, DirectX::XMVECTOR surfPalette, address originPttr) :
				   velocity(velo), pos(position),
				   angularVeloQtn(qtnAngularvelo), rotationQtn(qtnRotation),
				   scaleFactor{ scale, scale, scale, 1 },
				   dfType{ funcType, 0, 0, 0 },
				   surfRGBA(surfPalette),
				   origin{ (fourByteUnsigned)(((eightByteUnsigned)originPttr & 0xFFFFFFFF00000000) >> 32),
						   (fourByteUnsigned)(((eightByteUnsigned)originPttr & 0x00000000FFFFFFFF)), 0, 0 } {}

			// Where this figure is going + how quickly it's going
			// there
			DirectX::XMVECTOR velocity;

			// The location of this figure at any particular time
			DirectX::XMVECTOR pos;

			// How this figure is spinning, defined as radians/second
			// about an implicit angle
			DirectX::XMVECTOR angularVeloQtn;

			// The quaternion rotation applied to this figure at
			// any particular time
			DirectX::XMVECTOR rotationQtn;

			// Uniform object scale
			DirectX::XMFLOAT4 scaleFactor;

			// The distance function used to render [this]
			DirectX::XMUINT4 dfType;

			// Array of coefficients associated with the distance function
			// used to render [this]
			//float coeffs[10];

			// Fractal palette vector (bitmasked against function
			// properties to produce procedural colors)
			DirectX::XMVECTOR surfRGBA;

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

		// Get/set the angular velocity of [this]
		void BoostAngularVelo(DirectX::XMVECTOR angularVeloDelta);
		DirectX::XMVECTOR GetAngularVelo();

		// Get/Set the velocity of [this]
		void ApplyWork(DirectX::XMVECTOR veloDelta);
		DirectX::XMVECTOR GetVelo();

		// Get a write-allowed reference to the palette bitmask associated
		// with [this]
		DirectX::XMVECTOR& FetchSurfPalette();

		// Get a copy of the GPU-friendly [Figure] associated with [this]
		Figure GetCoreFigure();

		// Replace the [Figure] associated with [this] with one provided
		// externally (i.e. one from the GPU)
		void SetCoreFigure(Figure& fig);

	protected:
		// The actual data payload that travels to the GPU each frame
		Figure coreFigure;
};
