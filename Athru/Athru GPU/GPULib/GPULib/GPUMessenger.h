#pragma once

#include <d3d11.h>
#include "SceneFigure.h"
#include "AthruBuffer.h"
#include "ComputeShader.h"
#include <wrl\client.h>

class GPUMessenger
{
	public:
		GPUMessenger(const Microsoft::WRL::ComPtr<ID3D11Device>& device);
		~GPUMessenger();

		// Pass scene data along to the GPU
		void SceneToGPU(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
						SceneFigure::Figure* figures);

		// Pass the generic input buffer along to the GPU
		void CoreInputToGPU(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context, const DirectX::XMFLOAT4& sysOri);

		// Retrieve a gpu-read-only resource view of the
		// scene-data associated with [this]
		const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& GetGPUSceneView();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// A simple input struct that provides various per-frame
		// constants to the GPU
		struct GPUInput
		{
			DirectX::XMFLOAT4 tInfo; // Time info for each frame;
									 // delta-time in [x], current total time (seconds) in [y],
									 // frame count in [z], nothing in [w]
			DirectX::XMFLOAT4 systemOri; // Origin for the current star-system; useful for predicting figure
										 // positions during ray-marching/tracing (in [xyz], [w] is unused)
		};
		// ...And a reference to the buffer we'll need in order
		// to send that input data over to the GPU
		AthruGPU::AthruBuffer<GPUInput, AthruGPU::CBuffer> gpuInputBuffer;

		// A reference to the CPU-write/GPU-read buffer we'll use
		// to transfer per-object data across to the GPU at the
		// start of each frame
		AthruGPU::AthruBuffer<SceneFigure::Figure, AthruGPU::StrmBuffer> sceneBuf;
};