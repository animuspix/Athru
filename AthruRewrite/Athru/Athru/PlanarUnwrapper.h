#pragma once

#include <directxmath.h>

struct PlanarUnwrapper
{
	// Project the given point onto a plane aligned with XY space
	static DirectX::XMFLOAT4 Unwrap(DirectX::XMFLOAT4 minBoundingCoord, DirectX::XMFLOAT4 maxBoundingCoord,
									DirectX::XMFLOAT4 startingVert)
	{
		// Following unwrap algorithm found
	}
};

