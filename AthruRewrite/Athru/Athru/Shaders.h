#pragma once

#include "d3d11.h"
#include <d3dcompiler.h>
#include <directxmath.h>

class Shaders
{
	public:
		Shaders(ID3D11Device* device, HWND windowHandle);
		~Shaders();

		void Render(ID3D11DeviceContext* deviceContext,
					DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);

	private:
		struct Spatializer
		{
			DirectX::XMMATRIX world;
			DirectX::XMMATRIX view;
			DirectX::XMMATRIX projection;
		};

		void SetShaderParameters(ID3D11DeviceContext* deviceContext, 
								 DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);
		void RenderShader(ID3D11DeviceContext* deviceContext);

		ID3D11VertexShader* vertPlotter;
		ID3D11PixelShader* colorizer;
		ID3D11InputLayout* inputLayout;
		ID3D11Buffer* spatializer;
};

