#pragma once

#include <d3d11.h>
#include "ComputeShader.h"

class PathTracer : public ComputeShader
{
	public:
		PathTracer(LPCWSTR shaderFilePath);
		~PathTracer();
		void Dispatch(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
					  DirectX::XMVECTOR& cameraPosition,
					  DirectX::XMMATRIX& viewMatrix,
					  const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& displayTexWritable);

	private:
		// A simple input struct that provides various per-frame
		// constants to the GPU
		struct InputStuffs
		{
			DirectX::XMVECTOR cameraPos; // Camera position
			DirectX::XMMATRIX viewMat; // View matrix
			DirectX::XMMATRIX iViewMat; // Inverse view matrix
			DirectX::XMFLOAT4 deltaTime; // Change in time betweeen frames
			DirectX::XMUINT4 currTimeSecs; // Current time (in seconds)
			DirectX::XMUINT4 rendPassID; // Two-dimensional ID of the current render pass (for progressive rendering)
			DirectX::XMUINT4 numProgPatches; // Number of progressive slices to compute in each pass
			DirectX::XMUINT4 maxNumBounces; // Number of bounces for each ray
			DirectX::XMUINT4 numDirGaths; // Number of direct gathers (area light samples) for each ray
			DirectX::XMUINT4 numIndirGaths; // Number of indirect gathers (local samples) for each ray
		};

		// ...And a reference to the buffer we'll need in order
		// to send that input data over to the GPU
		Microsoft::WRL::ComPtr<ID3D11Buffer> shaderInputBuffer;

		// Anti-aliasing integration buffer, allows stratified samples to slowly integrate
		// into coherent images over time
		Microsoft::WRL::ComPtr<ID3D11Buffer> aaBuffer;

		// A GPU read/write allowed view of the AA integration buffer
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> aaBufferView;

		// A simple value representing the vertical/horizontal indices of the current
		// progressive render pass (first pixel row, second pixel row, first pixel
		// column, etc.)
		DirectX::XMUINT2 progPassCounters;
};
