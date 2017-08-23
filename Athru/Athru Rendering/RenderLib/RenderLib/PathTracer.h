#pragma once

#include <d3d11.h>
#include "ComputeShader.h"

class PathTracer : public ComputeShader
{
	public:
		PathTracer(LPCWSTR shaderFilePath);
		~PathTracer();
		void Dispatch(ID3D11DeviceContext* context);
		ID3D11ShaderResourceView* GetGICalcBufferReadable();

	private:
		// A local reference to the primary GI calculation buffer,
		// used to enable parallel GI by scattering ray contributions
		// across separate buffer indices
		ID3D11Buffer* giCalcBuffer;

		// A write-only shader-friendly view of the calculation buffer declared
		// above
		ID3D11UnorderedAccessView* giCalcBufferViewWritable;

		// A read-only shader-friendly view of the calculation buffer declared
		// above
		ID3D11ShaderResourceView* giCalcBufferViewReadable;
};

