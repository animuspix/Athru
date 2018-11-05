#pragma once

#include <DirectXMath.h>
#include "PhiloStrm.h"

// Concrete representation for light bounces after/during ray-marching;
// allows persistent path state during rendering (which occurs across
// multiple shading stages for performance)
struct LiPath
{
	PhiloStrm randStrm; // Stream state for the current path at the current bounce
	DirectX::XMFLOAT4 camInfo; // Camera ray-direction in [xyz], filter value in [w]
	DirectX::XMUINT bounceCtr; // Prefer incrementing this with [InterlockedAdd(...)]
	DirectX::XMFLOAT3 voluRGB; // Accumulated color from volumetric scattering along the path
	struct LiBounce
	{
		DirectX::XMUINT id; // Path ID for the current bounce (needed for shading)
		DirectX::XMFLOAT4x3 dirs; // Incoming/normal/outgoing (incoming synthesized during )
		DirectX::XMFLOAT4X4 mat; // Local material properties (including microfacet normal)
	};
	LiBounce bounces[GraphicsStuff::MAX_NUM_BOUNCES]; // Bounces contributing to [this]
};