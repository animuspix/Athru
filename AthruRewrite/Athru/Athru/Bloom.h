#pragma once

#include "Shader.h"

class Bloom : public Shader
{
	public:
		Bloom(ID3D11Device* device, HWND windowHandle,
			  LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath);
		~Bloom();
};

