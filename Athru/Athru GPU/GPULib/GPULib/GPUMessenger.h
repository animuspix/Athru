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
		void SceneToGPU(const Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
						SceneFigure::Figure* figures);

		// Retrieve a gpu-read-only resource view of the
		// scene-data associated with [this]
		const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& GetGPUSceneView();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// A reference to the CPU-write/GPU-read buffer we'll use
		// to transfer per-object data across to the GPU at the
		// start of each frame
		AthruBuffer<SceneFigure::Figure, GPGPUStuff::StrmBuffer> sceneBuf;
};