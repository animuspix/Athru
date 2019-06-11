#pragma once

#include <d3d12.h>
#include <functional>
#include "SceneFigure.h"
#include "AthruResrc.h"
#include "ComputePass.h"
#include <wrl\client.h>

class GPUMessenger
{
	public:
		GPUMessenger(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
                     AthruGPU::GPUMemory& gpuMem);
		~GPUMessenger();

		// Upload per-system scene data to the GPU
        // Per-planet upload is unimplemented atm, but expected once I have planetary UVs ready
		// All bulk uploads will probably occur here so I can supply a reasonable amount of data to
		// the copy command-list
		void SysToGPU(SceneFigure::Figure* system);

		// Pass per-frame inputs along to the GPU
		void InputsToGPU(const DirectX::XMFLOAT4& sysOri,
                         const Camera* camera);

		// Access data copied into the read-back buffer at a given offset, in a given format
		template<typename DataFmt>
		void DataFromGPU(u4Byte offs, u4Byte dataLen, DataFmt* readInto)
		{
			D3D12_RANGE range;
			range.Begin = offs;
			range.End = offs + dataLen;
			HRESULT hr = rdbkBuf.resrc->Map(0, &range, (void**)&gpuReadable);
			assert(SUCCEEDED(hr));
			memcpy(readInto, gpuReadable, dataLen);
			range.Begin = 0;
			range.End = 0;
			rdbkBuf.resrc->Unmap(0, &range);
		}

		// Acquire a const reference to the resource behind the readback buffer
		const Microsoft::WRL::ComPtr<ID3D12Resource>& AccessReadbackBuf();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// Per-frame constant inputs
		struct GPUInput
		{
			DirectX::XMFLOAT4 tInfo; // Time info for each frame;
									 // delta-time in [x], current total time (seconds) in [y],
									 // frame count in [z], nothing in [w]
			DirectX::XMFLOAT4 systemOri; // Origin for the current star-system; useful for predicting figure
										 // positions during ray-marching/tracing (in [xyz], [w] is unused)
			DirectX::XMVECTOR cameraPos; // Camera position in [xyz], [w] is unused
			DirectX::XMMATRIX viewMat; // View matrix
			DirectX::XMMATRIX iViewMat; // Inverse view matrix
			DirectX::XMFLOAT4 bounceInfo; // Maximum number of bounces in [x], number of supported surface BXDFs in [y],
										  // tracing epsilon value in [z]; [w] is unused
			DirectX::XMUINT4 resInfo; // Resolution info carrier; contains app resolution in [xy],
									  // AA sampling rate in [z], and display area in [w]
			DirectX::XMUINT4 tilingInfo; // Tiling info carrier; contains spatial tile counts in [x]/[y] and cumulative tile area in [z] ([w] is unused)
			DirectX::XMUINT4 tileInfo; // Per-tile info carrier; contains tile width/height in [x]/[y] and per-tile area in [z] ([w] is unused)
			DirectX::XMFLOAT4 excess; // DX12 constant buffers are 256-byte aligned; padding allocated here
		};
        // + CPU-side mapping point
        GPUInput* gpuInput;

		// System buffer
        AthruGPU::AthruResrc<SceneFigure::Figure,
                             AthruGPU::RResrc<AthruGPU::Buffer>> sysBuf;
		// Length of the system buffer, in bytes
		static constexpr u4Byte sysBytes = SceneStuff::ALIGNED_PARAMETRIC_FIGURES_PER_SYSTEM * sizeof(SceneFigure);

		// CPU->GPU messaging buffer
        AthruGPU::AthruResrc<uByte,
							 AthruGPU::MessageBuffer> msgBuf;

		// GPU->CPU readback buffer
		AthruGPU::AthruResrc<uByte,
							 AthruGPU::ReadbkBuffer> rdbkBuf;
		// + CPU-side mapping point
		uByte* gpuReadable;

		// Messaging command-list + allocator
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> msgCmds;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> msgAlloc;
};