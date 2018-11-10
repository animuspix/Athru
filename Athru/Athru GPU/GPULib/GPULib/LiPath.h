#pragma once

#include <DirectXMath.h>
#include "PhiloStrm.h"

// Concrete representation for light bounces after/during ray-marching;
// allows persistent path state during rendering (which occurs across
// multiple shading stages for performance)
struct LiBounce
{
	DirectX::XMUINT id; // Path ID for the current bounce (needed for shading)
	DirectX::XMFLOAT4x3 dirs; // Incoming/normal/outgoing
	DirectX::XMFLOAT3 iP; // Incident position
	DirectX::XMFLOAT3 eP; // Exitant position (usually equivalent to incident position, 
						  // variant for volumetric materials)
	DirectX::XMFLOAT4X4 mat; // Local material properties (including microfacet normal)
	DirectX::XMFLOAT3 rho; // Reflected surface color (only defined after shading)
	float pdf; // Probability of rays bouncing through sampled incoming directions
};

// Light bounces collect into path objects
struct LiPath
{
	PhiloStrm randStrm; // Stream state for the current path at the current bounce
	DirectX::XMFLOAT4 camInfo; // Camera ray-direction in [xyz], filter value in [w]
	DirectX::XMUINT bounceCtr; // Prefer incrementing this with [InterlockedAdd(...)]
	DirectX::XMFLOAT3 voluRGB; // Accumulated color from volumetric scattering along the path
	LiBounce vts[GraphicsStuff::MAX_NUM_BOUNCES]; // Bounces/vertices contributing to [this]
};