#pragma once

#include "Shader.h"

class Rasterizer : public Shader
{
	public:
		Rasterizer(ID3D11Device* device, HWND windowHandle,
				   LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath);
		~Rasterizer();
};

