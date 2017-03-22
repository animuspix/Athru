#pragma once

#include "Shader.h"

class DepthOfField : public Shader
{
	public:
		DepthOfField(ID3D11Device* device, HWND windowHandle,
					 LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath);
		~DepthOfField();
};

