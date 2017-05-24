#pragma once

#include "AthruTexture2D.h"

struct Heightfield
{
	AthruTexture2D lowerHemisphere;
	AthruTexture2D upperHemisphere;

	void ReleaseTextures()
	{
		// Release the heightfield associated with the
		// lower hemisphere

		lowerHemisphere.raw->Release();
		lowerHemisphere.raw = nullptr;

		lowerHemisphere.asRenderedShaderResource->Release();
		lowerHemisphere.asRenderedShaderResource = nullptr;

		lowerHemisphere.asStructuredBuffer->Release();
		lowerHemisphere.asStructuredBuffer = nullptr;

		lowerHemisphere.asComputeShaderResource->Release();
		lowerHemisphere.asComputeShaderResource = nullptr;

		// Release the heightfield associated with the
		// upper hemisphere

		upperHemisphere.raw->Release();
		upperHemisphere.raw = nullptr;

		upperHemisphere.asRenderedShaderResource->Release();
		upperHemisphere.asRenderedShaderResource = nullptr;

		upperHemisphere.asStructuredBuffer->Release();
		upperHemisphere.asStructuredBuffer = nullptr;

		upperHemisphere.asComputeShaderResource->Release();
		upperHemisphere.asComputeShaderResource = nullptr;
	}
};