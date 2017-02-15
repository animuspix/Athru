#pragma once

#include <d3d11.h>
#include <directxmath.h>
#include <d3dcompiler.h>

#include "Logger.h"

class ShaderCentre
{
	public:
		ShaderCentre(ID3D11Device* device, HWND windowHandle, Logger* logger);
		~ShaderCentre();

		bool Render(ID3D11DeviceContext* deviceContext, int indexCount, 
					DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);

	private:
		struct Spatializer
		{
			DirectX::XMMATRIX world;
			DirectX::XMMATRIX view;
			DirectX::XMMATRIX projection;
		};

		bool InitializeShader(ID3D11Device*, HWND, WCHAR*, WCHAR*);
		void ShutdownShader();

		bool SetShaderParameters(ID3D11DeviceContext* deviceContext, DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);
		void RenderShader(ID3D11DeviceContext*, int);
};

