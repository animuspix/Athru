#pragma once

#include <d3d11.h>
#include <directxmath.h>
#include "Typedefs.h"

class PipelineShader
{
	public:
		PipelineShader();
		~PipelineShader();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	protected:
		void BuildShader(ID3D11Device* device, HWND windowHandle,
						 D3D11_INPUT_ELEMENT_DESC* inputElementInfo, fourByteUnsigned numInputElements,
						 LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath);

		ID3D11VertexShader* vertShader;
		ID3D11PixelShader* pixelShader;
		ID3D11InputLayout* inputLayout;
};

