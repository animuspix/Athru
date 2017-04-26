#pragma once

#include <directxmath.h>
#include "MathIncludes.h"

struct NormalBuilder
{
	static DirectX::XMFLOAT4 BuildNormal(DirectX::XMFLOAT4 coreVert)
	{
		// All regular forms can be defined as subsets of the unit sphere
		// Points on the sphere (if defined relative to the origin) are
		// also valid normals
		// Thus, the normals of any regular shape will be equal to it's
		// normalized constituent points as long as the points are relative
		// to the local origin :D

		// Load the input FLOAT4 into an SSE register
		DirectX::XMVECTOR coreVertVector = DirectX::XMLoadFloat4(&coreVert);

		// Normalize it and store the result in [normalVector]
		DirectX::XMVECTOR normalVector = DirectX::XMVector3Normalize(coreVertVector);

		// Load back into a FLOAT4 register
		DirectX::XMFLOAT4 normalFloat;
		DirectX::XMStoreFloat4(&normalFloat, normalVector);

		// Reset the [w]-value to 1
		normalFloat.w = 1;

		// Return the generated normal
		return normalFloat;
	}
};