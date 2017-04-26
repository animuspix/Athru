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

		// Force render calls to go through here
		void Render(ID3D11DeviceContext* deviceContext,
					DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);

	private:
		struct EffectStatusBuffer
		{
			bool bloomActive;
			bool depthOfFieldActive;
			bool padding[14];
		};

		// Overload ordinary external shader render to do nothing
		// Also place the overload in [private] so it becomes inaccessible
		void Render(ID3D11DeviceContext* deviceContext,
					DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection,
					fourByteUnsigned numIndicesDrawing) {};

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

