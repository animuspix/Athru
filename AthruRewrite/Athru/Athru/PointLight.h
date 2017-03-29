#pragma once

#include "Shader.h"

class PointLight : public Shader
{
	public:
		PointLight(ID3D11Device* device, HWND windowHandle,
				   LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath);
		~PointLight();
};

