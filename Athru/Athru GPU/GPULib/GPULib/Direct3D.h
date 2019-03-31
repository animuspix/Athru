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
			const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& queue;
			u4Byte sel = (u4Byte)(critQueue);
			const Microsoft::WRL::ComPtr<ID3D12Fence>& fence = sync[sel];
			u8Byte fenceVal& = syncValues[sel];
			if constexpr (sel == 0)
			{ queue = graphicsQueue; }
			else if constexpr (sel == 1)
			{ queue = computeQueue; }
			else if constexpr (sel == 2)
			{ queue = copyQueue; }
			assert(SUCCEEDED(queue->Signal(fence.Get(), fenceVal)));
			assert(SUCCEEDED(fence->SetEventOnCompletion(syncValues[backBufferNdx * (sel + 1)], syncEvt[sel])));
			WaitForSingleObjectEx(syncEvt, INFINITE, false); // Allow infinite silent waiting for GPU operations
			fenceVal += 1; // Keep fence values consistent between CPU/GPU
		}

		// Retrieve a reference to the GPU memory manager
		AthruGPU::GPUMemory& GetGPUMem();

        // Map the swap-chain's backbuffer onto a D3D resource
        // Might edit this to return UWP display surface instead and control back-buffer copies myself
        void GetBackBuf(Microsoft::WRL::ComPtr<ID3D12Resource>& backBufTx);

		// Retrieve a reference to the active graphics pipeline-state & compute pipeline-state
		const Microsoft::WRL::ComPtr<ID3D12PipelineState>& GetGraphicsState();
		const Microsoft::WRL::ComPtr<ID3D12PipelineState>& GetComputeState();

		// Retrieve the device + main command queues without incrementing their reference
		// counts (also retrieve their command-lists + allocators)
		const Microsoft::WRL::ComPtr<ID3D12Device>& GetDevice();
		const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& GetGraphicsQueue(); // Path-tracing & image/mesh processing here
		const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& GetGraphicsCmdList();
		const Microsoft::WRL::ComPtr<ID3D12CommandAllocator> GetGraphicsCmdAllocator();
		const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& GetComputeQueue(); // Physics & ecosystem updates go here
		const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& GetComputeCmdList();
		const Microsoft::WRL::ComPtr<ID3D12CommandAllocator> GetComputeCmdAllocator();
		const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& GetCopyQueue(); // System->GPU or GPU->System copies (useful for UI & things like
																		  // radar, camera modes, etc.)
		const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& GetCopyCmdList();
		const Microsoft::WRL::ComPtr<ID3D12CommandAllocator> GetCopyCmdAllocator();

		// Retrieve a constant reference to information about the video adapter
		const DXGI_ADAPTER_DESC3& GetAdapterInfo();

	private:
		// Stores information about the video adapter
		DXGI_ADAPTER_DESC3 adapterInfo;

		// Swap-chain & device
		Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
		Microsoft::WRL::ComPtr<ID3D12Device> device;

		// Active graphics/compute pipeline-states
		Microsoft::WRL::ComPtr<ID3D12PipelineState> activeGraphicsState;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> activeComputeState;

		// Significant command queues & command lists
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> graphicsQueue;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> graphicsCmdList;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> computeQueue;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> computeCmdList;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> copyQueue;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> copyCmdList;

		// Graphics/compute/copy command-allocators
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> graphicsCmdAlloc;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> computeCmdAlloc;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> copyCmdAlloc;

		// The DX12 debug-device
		Microsoft::WRL::ComPtr<ID3D12Debug> debugDevice;

		// DX12 fence interfaces for GPU/CPU synchronization, also separate fence
		// values for each back-buffer surface + command-queue
		// + WinAPI event handles to capture fence updates
		const Microsoft::WRL::ComPtr<ID3D12Fence> sync[3];
		u8Byte syncValues[AthruGPU::NUM_SWAPCHAIN_BUFFERS * 3];
		HANDLE syncEvt[3];

		// Index of the current back-buffer surface at each frame
		uByte backBufferNdx;

		// Reference to Athru's GPU memory/residency manager
		AthruGPU::GPUMemory* gpuMem;
};

