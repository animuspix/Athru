#include "SceneFigure.h"

SceneFigure::SceneFigure()
{
	coreFigure = Figure(_mm_set_ps(0, 0, 0, 0), _mm_set_ps(1, 0, 0, 0),
						_mm_set_ps(0, 0, 0, 0), _mm_set_ps(0, 0, 0, 0), 1.0f,
						(fourByteUnsigned)FIG_TYPES::GRASS, _mm_set_ps(1, 1, 1, 1),
						this);
}

SceneFigure::SceneFigure(DirectX::XMVECTOR velo, DirectX::XMVECTOR position,
						 DirectX::XMVECTOR qtnAngularVelo, DirectX::XMVECTOR qtnRotation, float scale,
						 DirectX::XMVECTOR surfPalette, FIG_TYPES figType)
{
	coreFigure = Figure(velo, position, qtnAngularVelo, DirectX::XMQuaternionInverse(qtnRotation),
						scale, (fourByteUnsigned)figType, surfPalette,
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
	// Modify velocity delta according to mass, density,
	// etc. here
	coreFigure.velocity = _mm_add_ps(coreFigure.velocity, veloDelta);
}

DirectX::XMVECTOR SceneFigure::GetVelo()
{
	return coreFigure.velocity;
}

DirectX::XMVECTOR& SceneFigure::FetchSurfPalette()
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
}
