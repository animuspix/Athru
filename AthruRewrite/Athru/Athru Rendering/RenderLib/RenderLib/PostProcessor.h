#pragma once

#include "Shader.h"
#include "AthruTexture2D.h"

class PostProcessor : public Shader
{
	public:
		PostProcessor(ID3D11Device* device, HWND windowHandle,
					  LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath,
					  ID3D11ShaderResourceView* postProcessShaderResource);
		~PostProcessor();

		void Render(ID3D11DeviceContext* deviceContext,
					DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection,
					bool useBlur, bool useDrugs, bool brightenScene);

	private:
		struct EffectStatusBuffer
		{
			DirectX::XMINT4 blurActive;
			DirectX::XMINT4 drugsActive;
			DirectX::XMINT4 brightenActive;
		};

		// Rendering helper function, used to actually render out the
		// shader after the public [Render()] function passes it's
		// input values onto the pipeline
		void RenderShader(ID3D11DeviceContext * deviceContext);

		// A reference to the shader resource associated with the screen texture
		// (used to persist render information after the initial pass so that it
		// can be filtered and printed to the screen during post-processing)
		ID3D11ShaderResourceView* postProcessInputResource;

		// The state of the texture sampler used by [this]
		ID3D11SamplerState* wrapSamplerState;

		// Per-frame storage for effect statuses; used to control which
		// effects are/aren't expressed in the final render
		ID3D11Buffer* effectStatusBufferPttr;
};

