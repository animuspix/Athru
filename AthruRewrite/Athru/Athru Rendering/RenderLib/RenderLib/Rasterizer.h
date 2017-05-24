#pragma once

#include "PipelineShader.h"
#include "AthruGlobals.h"
#include "RenderManager.h"

class Rasterizer : public PipelineShader
{
	public:
		Rasterizer(ID3D11Device* device, HWND windowHandle,
				   LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath,
				   ID3D11ShaderResourceView* sceneColorShaderResource);
		~Rasterizer();

		// Pass the world/view/projection matrices + the scene texture onto the pipeline,
		// then render the voxel grid with [this]
		void Render(ID3D11DeviceContext* deviceContext,
					DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);

	private:
		struct MatBuffer
		{
			DirectX::XMMATRIX world;
			DirectX::XMMATRIX view;
			DirectX::XMMATRIX projection;
		};

		// Rendering helper function, used to actually render out the
		// voxel grid after the public [Render()] function passes it's
		// input values onto the pipeline
		void RenderShader(ID3D11DeviceContext* deviceContext);

		// A reference to the scene texture accessed by [this] per-frame
		ID3D11ShaderResourceView* sceneColorTexture;

		// The texture sampling state used with [this]
		ID3D11SamplerState* wrapSamplerState;

		// Per-frame storage for the world, view, and projection
		// matrices (needed for 3D projection during object shading)
		ID3D11Buffer* matBufferLocal;
};

