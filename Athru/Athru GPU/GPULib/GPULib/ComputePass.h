#pragma once

#include <d3d12.h>
#include <windows.h>
#include <wrl\client.h>
#include "GPUGlobals.h"
#include "AthruResrc.h"
#include "ResrcContext.h"

class ComputePass
{
	public:
		ComputePass(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
					HWND windowHandle,
					const char* shaderFilePath,
					const AthruGPU::RESRC_CTX shadingCtx,
					const u4Byte& numCBVs, const u4Byte& numSRVs, const u4Byte& numUAVs);
		~ComputePass();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	public:
		// Per-pass shader bytecode storage
		nsByte dxilMem[GraphicsStuff::SHADER_DXIL_ALLOC];
		// The resource context (root signature) associated with [this]
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig;
		// The pipeline state associated with [this]
		Microsoft::WRL::ComPtr<ID3D12PipelineState> shadingState;
};
