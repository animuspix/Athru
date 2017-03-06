#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>

class Shader
{
	public:
		Shader(ID3D11Device* device, HWND windowHandle,
			   LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath);
		~Shader();

		void Render(ID3D11DeviceContext* deviceContext,
					DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);

	protected:
		struct MatBuffer
		{
			DirectX::XMMATRIX world;
			DirectX::XMMATRIX view;
			DirectX::XMMATRIX projection;
		};

		void SetShaderParameters(ID3D11DeviceContext* deviceContext,
								 DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);
		void RenderShader(ID3D11DeviceContext* deviceContext);

		ID3D11VertexShader* vertShader;
		ID3D11PixelShader* pixelShader;
		ID3D11InputLayout* inputLayout;
		ID3D11Buffer* matBufferLocal;
};
