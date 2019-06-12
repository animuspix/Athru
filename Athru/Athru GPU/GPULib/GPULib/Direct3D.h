#pragma once

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <dxgi.h>
#include <dxgi1_6.h>
#include <d3dcommon.h>
#include <d3d12.h>
#include "GPUMemory.h"
#include <wrl/client.h>

class Direct3D
{
	public:
		Direct3D(HWND hwnd);
		~Direct3D();

		// Present generated imagery to the output monitor/Win64 surface
		void Present();

		// Execute a busy spinner until GPU work finishes for the compute/graphics/copy-queues
		template<D3D12_COMMAND_LIST_TYPE critQueue> // Should restrict this to [TYPE_DIRECT], [TYPE_COMPUTE], and [TYPE_COPY] with C++20
													// concepts
		void WaitForQueue()
		{
			Microsoft::WRL::ComPtr<ID3D12CommandQueue>& queue = graphicsQueue;
			constexpr u4Byte sel = (u4Byte)(critQueue);
			const Microsoft::WRL::ComPtr<ID3D12Fence>& fence = sync[sel];
			u8Byte& fenceVal = syncValues[sel];
			if constexpr (sel == 1)
			{ queue = computeQueue; }
			else if constexpr (sel == 2)
			{ queue = copyQueue; }
			assert(SUCCEEDED(queue->Signal(fence.Get(), fenceVal)));
			assert(SUCCEEDED(fence->SetEventOnCompletion(syncValues[sel], syncEvt[sel])));
			DWORD dw = WaitForSingleObject(syncEvt[sel], INFINITE); // Allow infinite silent waiting for GPU operations
			fenceVal = (fenceVal + 1) % 2; // Alternate fence values between [1] and [0]
		}

		// Retrieve a reference to the GPU memory manager
		AthruGPU::GPUMemory& GetGPUMem();

        // Map the swap-chain's backbuffer onto a D3D resource
        void GetBackBuf(const Microsoft::WRL::ComPtr<ID3D12Resource>& backBufTx, const u4Byte bufNdx);

		// Retrieve the device + main command queues without incrementing their reference
		// counts (also retrieve their command-lists + allocators)
		const Microsoft::WRL::ComPtr<ID3D12Device>& GetDevice();
		const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& GetGraphicsQueue(); // Path-tracing & image/mesh processing here
		const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& GetComputeQueue(); // Physics & ecosystem updates go here
		const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& GetCopyQueue(); // System->GPU or GPU->System copies (useful for UI & things like
																		  // radar, camera modes, etc.)
		// Retrieve a constant reference to information about the video adapter
		const DXGI_ADAPTER_DESC1& GetAdapterInfo();

	private:
		// Stores information about the video adapter
		DXGI_ADAPTER_DESC1 adapterInfo;

		// Swap-chain, device, DXGI factory, and DXGI debug interface
		Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
		Microsoft::WRL::ComPtr<ID3D12Device> device;

		// Graphics/compute/copy commmand-queues
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> graphicsQueue;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> computeQueue;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> copyQueue;

		// The DX12 debug layer
		Microsoft::WRL::ComPtr<ID3D12Debug> debugLayer;

		// DX12 fence interfaces for GPU/CPU synchronization, also separate fence
		// values for each back-buffer surface + command-queue
		// + WinAPI event handles to capture fence updates
		Microsoft::WRL::ComPtr<ID3D12Fence> sync[3];
		u8Byte syncValues[AthruGPU::NUM_SWAPCHAIN_BUFFERS];
		HANDLE syncEvt[3];

		// Index of the current back-buffer surface at each frame
		uByte backBufferNdx;

		// Reference to Athru's GPU memory/residency manager
		AthruGPU::GPUMemory* gpuMem;
};

