#pragma once

#include "d3d11.h"

struct AthruTexture
{
	AthruTexture() : 
		raw(nullptr), 
		asShaderResource(nullptr) {};

	ID3D11Texture2D* raw;
	ID3D11ShaderResourceView* asShaderResource;
};