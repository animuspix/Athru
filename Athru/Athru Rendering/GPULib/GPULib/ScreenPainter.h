#pragma once

#include "PipelineShader.h"
#include "AthruTexture2D.h"

class ScreenPainter : public PipelineShader
{
	public:
		ScreenPainter(ID3D11Device* device, HWND windowHandle,
					  LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath);
		~ScreenPainter();

		void Render(ID3D11DeviceContext* deviceContext,
					ID3D11ShaderResourceView* displayTexReadable);

	private:
		// Rendering helper function, used to actually render out the
		// shader after the public [Render()] function passes it's
		// input values onto the pipeline
		void RenderShader(ID3D11DeviceContext * deviceContext);

		// The texture sampler used by [this]
		ID3D11SamplerState* wrapSamplerState;
};

