#pragma once

#include <d3d11.h>
#include "ComputeShader.h"

class PathTracer : public ComputeShader
{
	public:
		PathTracer(LPCWSTR shaderFilePath);
		~PathTracer();
		void Dispatch(ID3D11DeviceContext* context,
					  DirectX::XMVECTOR& cameraPosition,
					  DirectX::XMMATRIX& viewMatrix);

		// Retrieve a reference to the GI calculation buffer (needed for
		// parallel ray emission + post-processed integration)
		ID3D11ShaderResourceView* GetGICalcBufferReadable();

	private:
		// A simple input struct that provides various per-frame
		// constants to the GPU
		struct InputStuffs
		{
			DirectX::XMVECTOR cameraPos;
			DirectX::XMMATRIX viewMat;
			DirectX::XMFLOAT4 deltaTime;
			DirectX::XMUINT4 currTimeSecs;
			DirectX::XMUINT4 rendPassID;
		};

		// ...And a reference to the buffer we'll need in order
		// to send that input data over to the GPU
		ID3D11Buffer* shaderInputBuffer;

		// A local reference to the display texture, used as the
		// raster target during ray-marching so that we can
		// easily post-process the results
		ID3D11UnorderedAccessView* displayTexture;

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

		// A simple value representing the index of the current
		// progressive render pass (first pixel row, second pixel
		// row, etc.)
		fourByteUnsigned progPassCounter;
};
