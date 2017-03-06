#pragma once

#include "Shader.h"

class PhongLighting : public Shader
{
	public:
		PhongLighting(ID3D11Device* device, HWND windowHandle,
					  LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath);
		~PhongLighting();

	private:
		void SetShaderParameters(ID3D11DeviceContext* deviceContext,
			 DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection,
			 DirectX::XMFLOAT4 ambientColor, DirectX::XMFLOAT4  solarColor);
};

