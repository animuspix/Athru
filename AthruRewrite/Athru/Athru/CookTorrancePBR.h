#pragma once

#include "Shader.h"

class CookTorrancePBR : public Shader
{
	public:
		CookTorrancePBR(ID3D11Device* device, HWND windowHandle,
						LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath);

		~CookTorrancePBR();

		
};

