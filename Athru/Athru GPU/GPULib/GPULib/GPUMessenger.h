#pragma once

#include <d3d11.h>
#include "SceneFigure.h"
#include <wrl\client.h>

class GPUMessenger
{
	public:
		GPUMessenger();
		~GPUMessenger();

		// Pass the current scene data along to the
		// GPU-friendly scene-data buffer
		void FrameStartSync(SceneFigure* sceneFigures);

		// Read the changes made to each element while it was
		// on the GPU back into it's original CPU-side
		// address
		void FrameEndSync();

		// Retrieve gpu-read-only/gpu-write-only resource views of the
		// scene-data-buffers associated with [this]
		const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& GetGPUReadableSceneView();
		const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& GetGPUWritableSceneView();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// A reference to the CPU-write/GPU-read buffer we'll use
		// to transfer per-object data across to the GPU at the
		// start of each frame
		Microsoft::WRL::ComPtr<ID3D11Buffer> figBufferInput;

		// A reference to the GPU-write-allowed buffer we'll use
		// to store changes we applied to the figure-data while
		// it was on the GPU
		Microsoft::WRL::ComPtr<ID3D11Buffer> figBufferOutput;

		// A reference to the staging buffer we'll use to
		// transfer per-object data back from the GPU to the
		// CPU
		Microsoft::WRL::ComPtr<ID3D11Buffer> figBufferStaging;

		// References to the unordered-access-views we'll use
		// to access the buffers declared above within the
		// physics/rendering/lighting shaders
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> gpuReadableSceneView;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> gpuWritableSceneView;
};