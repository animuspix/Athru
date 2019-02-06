#pragma once

#include <d3d12.h>
#include <directxmath.h>
#include "Typedefs.h"
#include <wrl\client.h>

class PipelineShader
{
	public:
		PipelineShader();
		~PipelineShader();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	protected:
		void BuildShader(const Microsoft::WRL::ComPtr<ID3D11Device>& device,
						 HWND windowHandle,
						 D3D11_INPUT_ELEMENT_DESC* inputElementInfo, u4Byte numInputElements,
						 LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath);
		Microsoft::WRL::ComPtr<ID3D11VertexShader> vertShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
};

