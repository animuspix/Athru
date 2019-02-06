#pragma once

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <dxgi.h>
#include <dxgi1_6.h>
#include <d3dcommon.h>
#include <d3d12.h>
#include <wrl/client.h>

class Direct3D
{
	public:
		Direct3D(HWND hwnd);
		~Direct3D();

		// Commit draw/dispatch calls to the GPU
		void Output();

		// Initialize presentation pipeline-state (only one raster pipeline in Athru, so no need to bundle full
		// pipeline state with shader objects)
		void InitRasterPipeline(const D3D12_SHADER_BYTECODE& vs,
								const D3D12_SHADER_BYTECODE& ps,
								const D3D12_INPUT_LAYOUT_DESC& inputLayout,
								ID3D12RootSignature* rootSig);

		// Retrieve the device + main command queues without incrementing their reference
		// counts (also retrieve their command-lists)
		const Microsoft::WRL::ComPtr<ID3D12Device>& GetDevice();
		const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& GetGraphicsQueue(); // Path-tracing & image/mesh processing here
		const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& GetGraphicsCmdList(); // Main command list paired to the graphics queue
		const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& GetComputeQueue(); // Physics & ecosystem updates go here
		const Microsoft::WRL::ComPtr<ID3D12CommandList>& GetComputeCmdList(); // Main command list paired to the compute queue
		const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& GetCopyQueue(); // System->GPU or GPU->System copies (useful for UI & things like
																		  // radar, camera modes, etc.)
		const Microsoft::WRL::ComPtr<ID3D12CommandList>& GetCopyCmdList(); // Main command list paired to the copy queue

		// Retrieve a constant reference to the render-target-view descriptor-heap
		const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& GetRTVDescHeap();

		// Retrieve a constant reference to the sampler descriptor-heap
		const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& GetGenericDescHeap();

		// Retrieve a constant reference to the generic descriptor heap (used with arbitrary shader inputs)
		const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& GetSamperDescHeap();

		// Retrieve a constant reference to information about the video adapter
		const DXGI_ADAPTER_DESC3& GetAdapterInfo();

	private:
		// Stores information about the video adapter
		DXGI_ADAPTER_DESC3 adapterInfo;

		// Swap-chain & device
		Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
		Microsoft::WRL::ComPtr<ID3D12Device> device;

		// Core/default descriptor heaps
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> genericHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> samplerHeap;

		// Significant command queues & command lists
		// Command allocators are carried by the GPU-side [StackAllocator]
		// (undefined atm)
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> graphicsQueue;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> graphicsCmdList;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> computeQueue;
		Microsoft::WRL::ComPtr<ID3D12CommandList> computeCmdList;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> copyQueue;
		Microsoft::WRL::ComPtr<ID3D12CommandList> copyCmdList;

		// Useful possible optimization - sparse normal maps!
		// - Generate coarse 2D normal maps along each positive/negative axis for each planet & each plant/animal species in each
		//   system visited by the player (stream between storage/cpu-memory/gpu-memory as needed)
		// - Each map encodes depth & normal direction per-pixel (axis mappings are implicit in the buffer ordering); sample points
		//   should use the standard surface SDFs and march surfaces down to the minimal epsilon before
		// - Maps should be high-res, but not 4K or higher (will maybe use 2048x2048 unordered-access-views)
		// - Successful scene intersections should still use sampled normals for shading; mapped normals are mainly a ray-marching
		//   thing
		// - Can cheaply measure approximate gradient at each point (by projecting into the cubemap), then interpolate sphere-tracing
		//   step distance between [0...2] according to the similarity between gradient and the incident ray (checked with the
		//   dot-product)
		// - Normal maps generated this way (basically normal cubemaps) can't easily adapt to concave geometry; likely best to adapt
		//   concave texture-mapping solutions (like some sort of numerical unwrap) instead
		// - Normal map generation likely to be super-expensive; possibly best to pre-generate and load them into CPU memory on
		//   startup

		// Reference to the standard render-target
		Microsoft::WRL::ComPtr<D3D12_GPU_DESCRIPTOR_HANDLE> defaultRenderTarget;
		Microsoft::WRL::ComPtr<ID3D12Debug> debugDevice;
};

