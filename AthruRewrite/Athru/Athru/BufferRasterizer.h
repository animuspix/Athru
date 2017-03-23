#pragma once

#include "Shader.h"

class BufferRasterizer : public Shader
{
	public:
		BufferRasterizer(ID3D11Device* device, HWND windowHandle,
						 LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath);
		~BufferRasterizer();
};

