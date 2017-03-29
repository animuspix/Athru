#pragma once

#include <directxmath.h>

struct PlanarUnwrapper
{
	// Project the given point onto a plane aligned with XY space
	static DirectX::XMFLOAT2 Unwrap(DirectX::XMFLOAT4 minBoundingCoord, DirectX::XMFLOAT4 maxBoundingCoord,
									DirectX::XMFLOAT4 boundingCoordRange, DirectX::XMFLOAT4 startingVert)
	{
		// Unwrap algorithm found on the devmaster.net forums; published by TheNut (see:
		// http://forum.devmaster.net/t/uvw-unwrap-algorithms/21409)

		DirectX::XMVECTOR vertLowerCoordDiff = _mm_sub_ps(DirectX::XMLoadFloat4(&startingVert), DirectX::XMLoadFloat4(&minBoundingCoord));
		DirectX::XMFLOAT4 outputUVW; // Prefer to do this inline, but [XMStoreFloat4] returns [void]
		DirectX::XMStoreFloat4(&outputUVW, _mm_div_ps(vertLowerCoordDiff, DirectX::XMLoadFloat4(&boundingCoordRange)));
		DirectX::XMFLOAT2 outputUV = DirectX::XMFLOAT2(outputUVW.x, outputUVW.y);
		return outputUV;
	}
};

