#pragma once

#include "d3d11.h"

struct AthruTexture2D
{
	// The raw texture data associated with [this]
	// Only useable with GPU pipeline shaders
	ID3D11Texture2D* raw;

	// A standard shader-friendly resource view, accessed
	// when the texture associated with [this] is being
	// passed into the GPU pipeline
	ID3D11ShaderResourceView* asRenderedShaderResource;

	// The texture data associated with [this], interpreted
	// as a Direct3D buffer (same type-of resource used for
	// storing vertices for geometry rendering) so it can
	// be used as input to compute-shaded pre-processing
	// functions
	ID3D11Buffer* asStructuredBuffer;

	// A compute-friendly resource view, accessed when
	// the texture associated with [this] is being used
	// as read-only input to a compute-shaded pre-processing
	// function
	ID3D11ShaderResourceView* asComputeShaderResource;

	// A compute-friendly resource view, accessed when
	// the texture associated with [this] is being used
	// as read/write input to a compute-shaded pre-processing
	// function
	ID3D11UnorderedAccessView* asWritableComputeShaderResource;
};