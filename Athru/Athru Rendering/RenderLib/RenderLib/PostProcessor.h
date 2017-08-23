#pragma once

#include <d3d11.h>
#include "ComputeShader.h"

class PostProcessor : public ComputeShader
{
	public:
		PostProcessor(LPCWSTR shaderFilePath);
		~PostProcessor();
		void Dispatch(ID3D11DeviceContext* context,
					  ID3D11ShaderResourceView* giCalcBufferReadable);
};
