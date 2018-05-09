#pragma once

#include "PipelineShader.h"
#include "AthruTexture2D.h"

class ScreenPainter : public PipelineShader
{
	public:
		ScreenPainter(const Microsoft::WRL::ComPtr<ID3D11Device>& device,
					  HWND windowHandle,
					  LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath);
		~ScreenPainter();

		void Render(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& deviceContext,
					const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& displayTexReadable);

	private:
		// The texture sampler used by [this]
		Microsoft::WRL::ComPtr<ID3D11SamplerState> wrapSamplerState;
};
