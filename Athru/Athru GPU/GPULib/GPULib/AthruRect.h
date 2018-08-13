#pragma once

#include <directxmath.h>
#include "PlanarUnwrapper.h"
#include "AthruGlobals.h"

template <typename VtType>
struct AthruRect
{
	AthruRect()
	{
		// Store min bounding cube position, max bounding cube position, and the difference (range) between them
		DirectX::XMFLOAT4 minBoundingPos = DirectX::XMFLOAT4(-1.0f, -1.0f, 0.1f, 1.0f);
		DirectX::XMFLOAT4 maxBoundingPos = DirectX::XMFLOAT4(1.0f, 1.0f, 0.1f, 1.0f);
		DirectX::XMFLOAT4 boundingRange = DirectX::XMFLOAT4(maxBoundingPos.x - minBoundingPos.x,
															maxBoundingPos.y - minBoundingPos.y,
															maxBoundingPos.z - minBoundingPos.z, 1);

		// Generate + cache vertex positions
		DirectX::XMFLOAT4 vert0Pos = DirectX::XMFLOAT4(-1.0f, 1.0f, 0.1f, 1.0f); // Front plane, upper left (v0)
		DirectX::XMFLOAT4 vert1Pos = DirectX::XMFLOAT4(1.0f, 1.0f, 0.1f, 1.0f); // Front plane, upper right (v1)
		DirectX::XMFLOAT4 vert2Pos = DirectX::XMFLOAT4(-1.0f, -1.0f, 0.1f, 1.0f); // Front plane, lower left (v2)
		DirectX::XMFLOAT4 vert3Pos = DirectX::XMFLOAT4(1.0f, -1.0f, 0.1f, 1.0f); // Front plane, lower right (v3)

		// Initialise vertices
		vts[0] = VtType(vert0Pos, // Upper left (v0)
						PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert0Pos));
		vts[1] = VtType(vert1Pos, // Upper right (v1)
			    	    PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert1Pos));
		vts[2] = VtType(vert2Pos, // Lower left (v2)
			    	    PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert2Pos));
		vts[3] = VtType(vert3Pos, // Lower right (v3)
			    	    PlanarUnwrapper::Unwrap(minBoundingPos, maxBoundingPos, boundingRange, vert3Pos));

		// Initialise indices
		// Each set of three values is one triangle
		indices[0] = 0;
		indices[1] = 1;
		indices[2] = 2;
		///////////////
		indices[3] = 2;
		indices[4] = 3;
		indices[5] = 1;
	}

	// Arbitrary-type vertices
	VtType vts[4];

	// Integer indices
	fourByteUnsigned indices[GraphicsStuff::SCREEN_RECT_INDEX_COUNT];
};