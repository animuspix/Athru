#pragma once

#include <DirectXMath.h>
#include "Typedefs.h"
#include "PhiloStrm.h"

// Concrete representation for light bounces after/during ray-marching;
// allows persistent path state during rendering (which occurs across
// multiple shading stages for performance)
struct LiBounce
{
	DirectX::XMUINT3 id; // Path ID for the current bounce (needed for shading); one-dimensional pixel
						 // offset in [x], two-dimensional index in [yz], [w] is unused
	DirectX::XMFLOAT4 dirs[3]; // Incoming/normal/outgoing/half
	DirectX::XMFLOAT3 iP; // Incident position
	DirectX::XMFLOAT3 eP; // Exitant position (usually equivalent to incident position,
						  // variant for volumetric materials)
	DirectX::XMFLOAT4X4 mat; // Local material properties (including microfacet normal)
	u4Byte figID; // Figure-ID at intersection
	u4Byte dfType; // Distance-function type at intersection
	DirectX::XMFLOAT4 gatherRGB; // Weighted (but not shaded) contribution from source-sampled
								 // next-event-estimation; includes gather-ray PDF in [w]
	DirectX::XMFLOAT4 gatherOcc; // Success value for source sampling in [x] (since checking [[float] == 0.0f]
								 // can be funky), sample direction in [yzw]
	DirectX::XMFLOAT2 etaIO; // Incident/outgoing indices-of-refraction
	// Philox strean for randomness during material synthesis (too few UAV bindings to #include
	// [randBuf] as well as material buffers)
	PhiloStrm randStrm;
};
