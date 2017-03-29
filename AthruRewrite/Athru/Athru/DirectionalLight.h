#pragma once

#include "Shader.h"

class DirectionalLight : public Shader
{
	public:
		DirectionalLight(ID3D11Device* device, HWND windowHandle,
						 LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath);
		~DirectionalLight();

		// Force render calls to go through here
		void Render(ID3D11DeviceContext* deviceContext,
					DirectX::XMFLOAT4 intensity, DirectX::XMFLOAT4 direction, DirectX::XMFLOAT4 tint, DirectX::XMFLOAT4 pos,
					DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection,
					ID3D11ShaderResourceView* texture, fourByteUnsigned numIndicesDrawing);

	private:
		struct LightBuffer
		{
			// Intensity is a scalar value, but keeping the buffer aligned is more efficient +
			// marginally safer, because you can be fairly sure that shader i/o won't (touch wood)
			// access beyond any particular element
			DirectX::XMFLOAT4 intensity;
			DirectX::XMFLOAT4 direction;
			DirectX::XMFLOAT4 tint;
			DirectX::XMFLOAT4 pos;
		};

		// Overload ordinary external shader render to do nothing
		// Also place the overload in [private] so it becomes inaccessible
		void Render(ID3D11DeviceContext* deviceContext,
					DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection,
					fourByteUnsigned numIndicesDrawing) {};

		// The state of the texture sampler used by [this]
		ID3D11SamplerState* wrapSamplerState;

		// Per-frame storage for light properties (intensity, direction, tint, and position;
		// strictly speaking directional lights don't have a positional component, but adding
		// one lets us do cool things like hyper-saturating the light source itself)
		ID3D11Buffer* lightBufferPttr;
};

