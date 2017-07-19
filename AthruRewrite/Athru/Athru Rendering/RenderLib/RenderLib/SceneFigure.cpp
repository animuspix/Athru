#include "SceneFigure.h"

SceneFigure::SceneFigure()
{
	eulerRotation = DirectX::XMFLOAT3(0, 0, 0);
	coreFigure = Figure(_mm_set_ps(0, 0, 0, 0), _mm_set_ps(1, 0, 0, 0),
						_mm_set_ps(0, 0, 0, 0), _mm_set_ps(0, 0, 0, 0), 1.0f,
						(fourByteUnsigned)DIST_FUNC_TYPES::SPHERE, false,
						false, 0.8f, 1.0f, 1.0f,
						_mm_set_ps(1.0f, 1.0f, 1.0f, 1.0f));
}

SceneFigure::SceneFigure(DirectX::XMVECTOR velo, DirectX::XMVECTOR position,
						 DirectX::XMVECTOR qtnAngularVelo, DirectX::XMFLOAT3 rotation, float scale,
						 DIST_FUNC_TYPES funcType, PHYS_BODY_TYPES physBodyType,
						 float givenRigidity, float givenMass, float givenDensity,
						 DirectX::XMVECTOR surfColor)
{
	// Split the given body type into separate static/non-static and rigid-/soft-body
	// properties
	// All static elements are assumed to be rigidbodies
	bool isStatic;
	bool isRigid;

	if (physBodyType == PHYS_BODY_TYPES::RIGID)
	{
		isStatic = false;
		isRigid = true;
	}

	else if (physBodyType == PHYS_BODY_TYPES::SOFT)
	{
		isStatic = false;
		isRigid = false;
	}

	else
	{
		isStatic = true;
		isRigid = true;

		// Static rigid-bodies cannot have velocity in any direction
		velo = _mm_set_ps(0, 0, 0, 0);
		qtnAngularVelo = _mm_set_ps(1, 0, 0, 0);
	}

	// Convert the given rotation into the internal [Figure]'s
	// quaternion representation; also invert it beforehand since
	// rotating rays is much easier than rotating abstract
	// distance functions
	eulerRotation = rotation;
	DirectX::XMVECTOR invPitchYawRoll = DirectX::XMQuaternionRotationRollPitchYaw(eulerRotation.x,
																				  eulerRotation.y,
																				  eulerRotation.z);

	coreFigure = Figure(velo, position, qtnAngularVelo, DirectX::XMQuaternionInverse(invPitchYawRoll),
						scale, (fourByteUnsigned)funcType, isStatic, isRigid, givenRigidity,
						givenMass, givenDensity, surfColor);
}

SceneFigure::~SceneFigure()
{
}

void SceneFigure::SetDistFuncType(DIST_FUNC_TYPES funcType)
{
	coreFigure.dfType.x = (fourByteUnsigned)funcType;
}

DIST_FUNC_TYPES SceneFigure::GetDistFuncType()
{
	return (DIST_FUNC_TYPES)coreFigure.dfType.x;
}

void SceneFigure::SetRotation(DirectX::XMFLOAT3& pitchYawRoll)
{
	// Convert the given rotation into the internal [Figure]'s
	// quaternion representation; also invert it beforehand since
	// rotating rays is much easier than rotating abstract
	// distance functions
	DirectX::XMVECTOR invPitchYawRoll = DirectX::XMQuaternionRotationRollPitchYaw(pitchYawRoll.x,
																				  pitchYawRoll.y,
																				  pitchYawRoll.z);
	coreFigure.rotationQtn = DirectX::XMQuaternionInverse(invPitchYawRoll);
}

DirectX::XMFLOAT3 SceneFigure::GetEulerRotation()
{
	return eulerRotation;
}

void SceneFigure::BoostAngularVelo(DirectX::XMVECTOR angularVeloDelta)
{
	coreFigure.angularVeloQtn = DirectX::XMQuaternionMultiply(coreFigure.angularVeloQtn, angularVeloDelta);
}

DirectX::XMVECTOR SceneFigure::GetAngularVelo()
{
	return coreFigure.angularVeloQtn;
}

void SceneFigure::ApplyWork(DirectX::XMVECTOR veloDelta)
{
	if (!coreFigure.isStatic.x)
	{
		// More work is needed to shift heavier masses; reduce the velocity delta
		// appropriately
		veloDelta = _mm_div_ps(veloDelta, _mm_set_ps(coreFigure.mass.x,
													 coreFigure.mass.x,
													 coreFigure.mass.x,
													 coreFigure.mass.x));

		coreFigure.velocity = _mm_add_ps(coreFigure.velocity, veloDelta);
	}
}

DirectX::XMVECTOR SceneFigure::GetVelo()
{
	return coreFigure.velocity;
}

void SceneFigure::SetBodyType(PHYS_BODY_TYPES bodyType)
{
	// Split the given body type into separate static/non-static and rigid-/soft-body
	// properties
	if (bodyType == PHYS_BODY_TYPES::RIGID)
	{
		coreFigure.isStatic.x = false;
		coreFigure.isRigid.x = true;
	}

	else if (bodyType == PHYS_BODY_TYPES::SOFT)
	{
		coreFigure.isStatic.x = false;
		coreFigure.isRigid.x = false;
	}

	else
	{
		coreFigure.isStatic.x = true;
		coreFigure.isRigid.x = true;

		// Static rigid-bodies cannot have velocity in any direction
		coreFigure.velocity = _mm_set_ps(0, 0, 0, 0);
		coreFigure.angularVeloQtn = _mm_set_ps(1, 0, 0, 0);
	}
}

PHYS_BODY_TYPES SceneFigure::GetBodyType()
{
	if (coreFigure.isRigid.x &&
		!coreFigure.isStatic.x)
	{
		return PHYS_BODY_TYPES::RIGID;
	}

	else if (!coreFigure.isRigid.x &&
			 !coreFigure.isStatic.x)
	{
		return PHYS_BODY_TYPES::SOFT;
	}

	if (coreFigure.isStatic.x)
	{
		return PHYS_BODY_TYPES::STATIC;
	}

	// Assume figures that somehow fail all of the
	// above are rigid-bodies
	return PHYS_BODY_TYPES::RIGID;
}

void SceneFigure::SetRigidity(float givenRigidity)
{
	// Clamp the given rigidity value to the range 1...0.0f
	givenRigidity = max(givenRigidity - min((givenRigidity > 1.0f) - 1.0f, 0.0f), 0.0f);

	// Pass the given rigidity value along to the actual
	// [Figure] that [this] is defined to interface
	// with
	coreFigure.rigidity.x = givenRigidity;

	// If the given rigidity is less than one, [this] will behave
	// like a non-static soft-body
	if (givenRigidity < 1.0f)
	{
		coreFigure.isStatic.x = false;
		coreFigure.isRigid.x = false;
	}

	// If the given rigidity is equal to one, [this] will behave like
	// a rigidbody and may or may not be static
	else
	{
		coreFigure.isRigid.x = true;
	}
}

float SceneFigure::GetRigidity()
{
	return coreFigure.rigidity.x;
}

float& SceneFigure::FetchMass()
{
	return coreFigure.mass.x;
}

float& SceneFigure::FetchDensity()
{
	return coreFigure.density.x;
}

DirectX::XMVECTOR& SceneFigure::FetchSurfColor()
{
	return coreFigure.surfRGBA;
}

SceneFigure::Figure SceneFigure::GetCoreFigure()
{
	return coreFigure;
}

void SceneFigure::SetCoreFigure(Figure& fig)
{
	// Store the given core figure
	coreFigure = fig;

	// Update the high-level (euler) rotation to
	// match the quaternion associated with the given
	// figure
}
