#pragma once

#include "Shader.h"

class SpotLight : public Shader
{
	public:
		SpotLight(ID3D11Device* device, HWND windowHandle,
				  LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath);
		~SpotLight();
};

