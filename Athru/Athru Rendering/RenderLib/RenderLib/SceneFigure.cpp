#include "SceneFigure.h"

SceneFigure::SceneFigure()
{
	DirectX::XMVECTOR baseDistCoeffs[10] = { _mm_set_ps(1, 1, 1, 1), _mm_set_ps(0, 0, 0, 0), _mm_set_ps(0, 0, 0, 0),
											 _mm_set_ps(0, 0, 0, 0), _mm_set_ps(1, 1, 1, 1), _mm_set_ps(0, 0, 0, 0),
										     _mm_set_ps(0, 0, 0, 0), _mm_set_ps(0, 0, 0, 0), _mm_set_ps(1, 1, 1, 1),
											 _mm_set_ps(0, 0, 0, 0) };

	DirectX::XMVECTOR baseRGBACoeffs[10] = { _mm_set_ps(1, 1, 1, 1), _mm_set_ps(0, 0, 0, 0), _mm_set_ps(0, 0, 0, 0),
											 _mm_set_ps(0, 0, 0, 0), _mm_set_ps(1, 1, 1, 1), _mm_set_ps(0, 0, 0, 0),
											 _mm_set_ps(0, 0, 0, 0), _mm_set_ps(0, 0, 0, 0), _mm_set_ps(1, 1, 1, 1),
											 _mm_set_ps(0, 0, 0, 0) };

	coreFigure = Figure(_mm_set_ps(0, 0, 0, 0), _mm_set_ps(1, 0, 0, 0),
						_mm_set_ps(0, 0, 0, 0), _mm_set_ps(0, 0, 0, 0), 1.0f,
						(fourByteUnsigned)FIG_TYPES::CRITTER, baseDistCoeffs,
						baseRGBACoeffs, 0,
						this);
}

SceneFigure::SceneFigure(DirectX::XMVECTOR velo, DirectX::XMVECTOR position,
						 DirectX::XMVECTOR qtnAngularVelo, DirectX::XMVECTOR qtnRotation, float scale,
						 fourByteUnsigned figType, DirectX::XMVECTOR* distCoeffs,
						 DirectX::XMVECTOR* rgbaCoeffs, fourByteUnsigned isNonNull)
{
	coreFigure = Figure(velo, position, qtnAngularVelo, DirectX::XMQuaternionInverse(qtnRotation),
						scale, (fourByteUnsigned)figType, distCoeffs, rgbaCoeffs, isNonNull,
						this);
}

SceneFigure::~SceneFigure()
{
}

FIG_TYPES SceneFigure::GetDistFuncType()
{
	return (FIG_TYPES)coreFigure.dfType.x;
}

void SceneFigure::SetRotation(DirectX::XMVECTOR axis,
							  float angle)
{
	// Convert the given rotation into the internal [Figure]'s
	// quaternion representation; also invert it beforehand since
	// rotating rays is much easier than rotating abstract
	// distance functions
	DirectX::XMVECTOR invAxisAngle = DirectX::XMQuaternionRotationAxis(axis,
																	   angle);

	coreFigure.rotationQtn = DirectX::XMQuaternionInverse(invAxisAngle);
}

DirectX::XMVECTOR SceneFigure::GetQtnRotation()
{
	return coreFigure.rotationQtn;
}

//void SceneFigure::BoostAngularVelo(DirectX::XMVECTOR angularVeloDelta)
//{
//	coreFigure.angularVeloQtn = DirectX::XMQuaternionMultiply(coreFigure.angularVeloQtn, angularVeloDelta);
//}
//
//DirectX::XMVECTOR SceneFigure::GetAngularVelo()
//{
//	return coreFigure.angularVeloQtn;
//}
//
//void SceneFigure::ApplyWork(DirectX::XMVECTOR veloDelta)
//{
//	// Modify velocity delta according to mass, density,
//	// etc. here
//	coreFigure.velocity = _mm_add_ps(coreFigure.velocity, veloDelta);
//}
//
//DirectX::XMVECTOR SceneFigure::GetVelo()
//{
//	return coreFigure.velocity;
//}

DirectX::XMVECTOR* SceneFigure::GetRGBACoeffs()
{
	return coreFigure.rgbaCoeffs;
}

SceneFigure::Figure SceneFigure::GetCoreFigure()
{
	return coreFigure;
}

void SceneFigure::SetCoreFigure(Figure& fig)
{
	// Store the given core figure
	coreFigure = fig;
}
