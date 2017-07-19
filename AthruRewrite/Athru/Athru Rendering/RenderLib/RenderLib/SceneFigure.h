#pragma once

#include <directxmath.h>
#include "UtilityServiceCentre.h"

enum class DIST_FUNC_TYPES
{
	SPHERE,
	CUBE
};

enum class PHYS_BODY_TYPES
{
	RIGID,
	SOFT,
	STATIC
};

class SceneFigure
{
	public:
		SceneFigure();
		SceneFigure(DirectX::XMVECTOR velo, DirectX::XMVECTOR position,
				    DirectX::XMVECTOR qtnAngularVelo, DirectX::XMFLOAT3 rotation, float scale,
					DIST_FUNC_TYPES funcType,
					PHYS_BODY_TYPES physBodyType, float givenRigidity, float givenMass,
					float givenDensity,
					DirectX::XMVECTOR surfColor);
		~SceneFigure();

		// GPU-friendly version of [this]; should only be accessed
		// indirectly through the interfaces described below
		struct Figure
		{
			Figure() {}
			Figure(DirectX::XMVECTOR velo, DirectX::XMVECTOR position,
				   DirectX::XMVECTOR qtnAngularvelo, DirectX::XMVECTOR qtnRotation, float scale,
				   fourByteUnsigned funcType, bool staticNess, bool rigidNess,
				   float givenRigidity, float givenMass, float givenDensity,
				   DirectX::XMVECTOR surfColor) :
				   velocity(velo), pos(position),
				   angularVeloQtn(qtnAngularvelo), rotationQtn(qtnRotation),
				   scaleFactor{ scale, scale, scale, scale },
				   dfType{ funcType, 0, 0, 0 },
				   isStatic{ staticNess, 0, 0, 0 },
				   isRigid{ rigidNess, 0, 0, 0 },
				   rigidity{ givenRigidity, 0, 0, 0 },
				   mass{ givenMass, 0, 0, 0 },
				   density{ givenDensity, 0, 0, 0 },
				   surfRGBA(surfColor) {}

			// Where this figure is going + how quickly it's going
			// there
			DirectX::XMVECTOR velocity;

			// The location of this figure at any particular time
			DirectX::XMVECTOR pos;

			// How this figure is spinning, defined as radians/second
			// about an implicit angle
			DirectX::XMVECTOR angularVeloQtn;

			// The quaternion rotationQtn applied to this figure at
			// any particular time
			DirectX::XMVECTOR rotationQtn;

			// Uniform object scale
			DirectX::XMFLOAT4 scaleFactor;

			// Simple switch for now; consider allowing blends in
			// future versions
			DirectX::XMUINT4 dfType;

			// Whether this figure is/isn't a static physics object
			// (static physics objects reflect energy realistically,
			// but don't experience displacement or deformation
			// themselvse)
			DirectX::XMUINT4 isStatic;

			// Whether this is/isn't a rigid-body physics object
			// (rigid-bodies absorb and reflect energy without
			// experiencing deformation; objects that aren't rigid-bodies
			// are soft-bodies, which experience elastic deformation)
			// (rigid deformation (i.e. fracture) is too much of an
			// edge-case to justify implementing it atm)
			DirectX::XMUINT4 isRigid;

			// Object rigidity (1.0f for rigid-bodies, 0...0.9f for
			// soft-bodies)
			DirectX::XMFLOAT4 rigidity;

			// Generic physical properties
			DirectX::XMFLOAT4 mass;
			DirectX::XMFLOAT4 density;

			// Assumed average surface color
			DirectX::XMVECTOR surfRGBA;
		};

		// Set the underlying distance function used to
		// define the shape of [this]
		void SetDistFuncType(DIST_FUNC_TYPES funcType);

		// Retrieve the underlying distance function used
		// to define the shape of [this]
		DIST_FUNC_TYPES GetDistFuncType();

		// Rotate [this] by the given angles about
		// x, y, and z
		void SetRotation(DirectX::XMFLOAT3& pitchYawRoll);

		// Get the euler-angle rotation associated with [this]
		DirectX::XMFLOAT3 GetEulerRotation();

		// Get/set the angular velocity of [this]
		void BoostAngularVelo(DirectX::XMVECTOR angularVeloDelta);
		DirectX::XMVECTOR GetAngularVelo();

		// Get/Set the velocity of [this]
		void ApplyWork(DirectX::XMVECTOR veloDelta);
		DirectX::XMVECTOR GetVelo();

		// Get/set the physics behaviour (rigid/soft/static)
		// associated with [this]
		void SetBodyType(PHYS_BODY_TYPES bodyType);
		PHYS_BODY_TYPES GetBodyType();

		// Get/set the rigidity of [this] (1.0f for rigid-bodies and
		// static-bodies, 0.1...0.9f for soft-bodies)
		void SetRigidity(float givenRigidity);
		float GetRigidity();

		// Get a write-allowed reference to the mass of [this]
		float& FetchMass();

		// Get a write-allowed reference to the density of [this]
		float& FetchDensity();

		// Get a write-allowed reference to the assumed average surface color of
		// [this]
		// Color should be procedurally defined in the future, this is just a
		// a simplification so I don't have to spend ages finding reasonable
		// color algorithms :P
		DirectX::XMVECTOR& FetchSurfColor();

		// Get a copy of the GPU-friendly [Figure] associated with [this]
		Figure GetCoreFigure();

		// Replace the [Figure] associated with [this] with one provided
		// externally (i.e. one from the GPU)
		void SetCoreFigure(Figure& fig);

	private:
		// The per-axis representation of the rotationQtn associated with
		// [this]
		DirectX::XMFLOAT3 eulerRotation;

		// The actual data payload that travels to the GPU each frame
		Figure coreFigure;
};
