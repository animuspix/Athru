#pragma once

#include <d3d11.h>

struct AthruTexture2D
{
	// The raw texture data associated with [this]
	ID3D11Texture2D* raw;

	// A read-only shader-friendly resource view
	ID3D11ShaderResourceView* asReadOnlyShaderResource;

	// A write-allowed shader-friendly resource view
	ID3D11UnorderedAccessView* asWritableShaderResource;
};