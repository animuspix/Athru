#pragma once

#include <directxmath.h>

struct SQT
{
	SQT() : rotationQuaternion(DirectX::XMQuaternionRotationRollPitchYaw(0, 0, 0)),
			pos(_mm_set_ps(1, 0, 0, 0)),
			uniformScale(_mm_set_ps(1, 1, 1, 1)) {}

	SQT(DirectX::XMVECTOR baseRotationQuaternion,
		DirectX::XMVECTOR basePos,
		float baseScale) :
		rotationQuaternion(baseRotationQuaternion),
		pos(basePos),
		uniformScale(_mm_set_ps(1, baseScale, baseScale, baseScale)) {}

	DirectX::XMMATRIX asMatrix()
	{
		return DirectX::XMMatrixAffineTransformation(uniformScale,
													 _mm_set_ps(0, 0, 0, 0),
													 rotationQuaternion,
													 pos);
	}

	DirectX::XMVECTOR rotationQuaternion;
	DirectX::XMVECTOR pos;
	DirectX::XMVECTOR uniformScale;
};

