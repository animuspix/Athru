#pragma once

#include <d3d11.h>
#include "SceneFigure.h"

class GPUScene
{
	public:
		GPUScene();
		~GPUScene();

		// Pass the current scene data along to the
		// GPU-friendly scene-data buffer
		void CommitSceneToGPU(SceneFigure* sceneFigures, byteUnsigned figCount);

		// Read any changes made to the data while it was
		// on the GPU to a CPU-side array that can be 
		// accessed by the rest of the engine
		SceneFigure* RetrieveChangesFromGPU();

		// Retrieve gpu-read-only/gpu-write-only resource views of the
		// scene-data-buffers associated with [this]
		ID3D11ShaderResourceView* GetGPUReadableSceneView();
		ID3D11UnorderedAccessView* GetGPUWritableSceneView();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// The number of figures defined on the GPU
		byteUnsigned numFigures;

		// A reference to the CPU-write/GPU-read buffer we'll use
		// to transfer per-object data across to the GPU at the
		// start of each frame
		ID3D11Buffer* figBufferInput;

		// A reference to the GPU-write-allowed buffer we'll use
		// to store changes we applied to the figure-data while
		// it was on the GPU
		ID3D11Buffer* figBufferOutput;

		// A reference to the staging buffer we'll use to
		// transfer per-object data back from the GPU to the
		// CPU
		ID3D11Buffer* figBufferStaging;

		// References to the unordered-access-views we'll use
		// to access the buffers declared above within the
		// physics/rendering/lighting shaders
		ID3D11ShaderResourceView* gpuReadableSceneView;
		ID3D11UnorderedAccessView* gpuWritableSceneView;
};