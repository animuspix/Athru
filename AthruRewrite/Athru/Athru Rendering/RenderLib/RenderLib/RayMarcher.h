#pragma once

#include <d3d11.h>
#include "ComputeShader.h"

class RayMarcher : public ComputeShader
{
	public:
		RayMarcher(LPCWSTR shaderFilePath);
		~RayMarcher();
		void Dispatch(ID3D11DeviceContext* context,
					  DirectX::XMVECTOR& cameraPosition,
					  DirectX::XMMATRIX& viewMatrix,
					  ID3D11ShaderResourceView* gpuReadableSceneDataView,
					  ID3D11UnorderedAccessView* gpuWritableSceneDataView,
					  fourByteUnsigned numFigures);

	private:
		// Very simple input struct, used to expose delta-time and
		// camera position to the GPU-side shader program
		struct InputStuffs
		{
			DirectX::XMVECTOR cameraPos;
			DirectX::XMMATRIX viewMat;
			DirectX::XMFLOAT4 deltaTime;
			DirectX::XMUINT4 currTimeSecs;
			DirectX::XMUINT4 numFigures;
			DirectX::XMUINT4 rendPassID;
		};

		// ...And a reference to the buffer we'll need in order
		// to send that input data over to the GPU
		ID3D11Buffer* shaderInputBuffer;

		// A local reference to the display texture, used as the
		// raster target during ray-marching so that we can
		// easily post-process the results
		ID3D11UnorderedAccessView* displayTexture;

		// A simple value representing the index of the current
		// progressive render pass (first pixel row, second pixel
		// row, etc.)
		fourByteUnsigned progPassCounter;
};

