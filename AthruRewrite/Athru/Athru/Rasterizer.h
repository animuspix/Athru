#pragma once

#include "Shader.h"

#define AMBIENT_DIFFUSE_RATIO 0.4f

class Rasterizer : public Shader
{
	public:
		Rasterizer(ID3D11Device* device, HWND windowHandle,
				   LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath);
		~Rasterizer();

		// Force render calls to go through here
		void Render(ID3D11DeviceContext* deviceContext,
					DirectX::XMFLOAT4 intensity, DirectX::XMFLOAT4 direction, DirectX::XMFLOAT4 diffuse, DirectX::XMFLOAT4 ambient, DirectX::XMFLOAT4 pos,
					DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection,
					ID3D11ShaderResourceView* texture, fourByteUnsigned numIndicesDrawing);

	private:
		struct LightBuffer
		{
			// Intensity is a scalar value, but keeping the buffer aligned is more efficient +
			// marginally safer, because you can be fairly sure that shader i/o won't (touch wood)
			// access beyond any particular element
			DirectX::XMFLOAT4 dirIntensity;
			DirectX::XMFLOAT4 dirDirection;
			DirectX::XMFLOAT4 dirDiffuse;
			DirectX::XMFLOAT4 dirAmbient;
			DirectX::XMFLOAT4 dirPos;
			// Point/spot light data here...
			DirectX::XMFLOAT4 viewVec;
		};

		// Overload ordinary external shader render to do nothing
		// Also place the overload in [private] so it becomes inaccessible
		void Render(ID3D11DeviceContext* deviceContext,
					DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection,
					fourByteUnsigned numIndicesDrawing) {};

		// The state of the texture sampler used by [this]
		ID3D11SamplerState* wrapSamplerState;

		// Per-frame storage for light properties (intensity, direction, diffuse/ambient tint,
		// and position; strictly speaking directional lights don't have a positional component, but adding
		// one heavily simplifies the math for diffuse/specular PBR)
		ID3D11Buffer* lightBufferPttr;
};

