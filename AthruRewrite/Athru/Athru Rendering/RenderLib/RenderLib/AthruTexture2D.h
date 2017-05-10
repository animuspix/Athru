#pragma once

#include "d3d11.h"

struct AthruTexture2D
{
	ID3D11Texture2D* raw;
	ID3D11ShaderResourceView* asRenderedShaderResource;
};