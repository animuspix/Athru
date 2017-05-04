#pragma once

#define VERTEX_PROPERTY_COUNT 9
#define ANIM_DURATION_SECONDS 2

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>

#include "Typedefs.h"

class Shader
{
	public:
		Shader(ID3D11Device* device, HWND windowHandle,
			   LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath);
		~Shader();

		virtual void Render(ID3D11DeviceContext* deviceContext,
							DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection,
							fourByteUnsigned numIndicesDrawing);

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	protected:
		struct MatBuffer
		{
			DirectX::XMMATRIX world;
			DirectX::XMMATRIX view;
			DirectX::XMMATRIX projection;
			DirectX::XMFLOAT4 animTimeStep;
		};

		virtual void SetShaderParameters(ID3D11DeviceContext* deviceContext,
										 DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);

		virtual void RenderShader(ID3D11DeviceContext* deviceContext, fourByteUnsigned numIndicesDrawing);

		ID3D11VertexShader* vertShader;
		ID3D11PixelShader* pixelShader;
		ID3D11InputLayout* inputLayout;
		ID3D11Buffer* matBufferLocal;

	private:
		enum class ACCUMULATOR_DIRECTION
		{
			RISING = 1,
			FALLING = -1
		};

		float dtAccumulator;
		ACCUMULATOR_DIRECTION accumulatorGrowth;
};

