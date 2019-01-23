#pragma once

#include <d3d11.h>
#include <wrl/client.h>

struct AthruTexture2D
{
	// The raw texture data associated with [this]
	Microsoft::WRL::ComPtr<ID3D11Texture2D> raw;

	// A read-only shader-friendly resource view
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> readOnlyView;

	// A write-allowed shader-friendly resource view
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> readWriteView;
};