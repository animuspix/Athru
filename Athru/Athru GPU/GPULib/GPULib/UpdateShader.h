#pragma once

#include "ComputeShader.h"

class UpdateShader : public ComputeShader
{
	public:
		UpdateShader(LPCWSTR shaderFilePath);
		~UpdateShader();
		void Dispatch(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
					  fourByteUnsigned threadsX,
					  fourByteUnsigned threadsY,
					  fourByteUnsigned threadsZ);
};

