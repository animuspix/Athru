#pragma once

#include "Shader.h"

class TexturedRasterizer : public Shader
{
	public:
		TexturedRasterizer(ID3D11Device* device, HWND windowHandle,
						   LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath);
		~TexturedRasterizer();

		// Force render calls to go through here
		void Render(ID3D11DeviceContext* deviceContext,
					DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection,
					ID3D11ShaderResourceView* texture);

	private:
		// Overload ordinary shader render to do nothing
		// Also place the overload in [private] so it becomes inaccessible
		void Render(ID3D11DeviceContext* deviceContext,
					DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection) {};

		ID3D11SamplerState* wrapSamplerState;
};

