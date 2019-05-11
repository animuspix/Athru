#pragma once

#include <d3d12.h>
#include <functional>
#include "AthruResrc.h"
#include "PixHistory.h"
#include "ComputePass.h"
#include "Camera.h"
#include "LiBounce.h"
#include "ResrcContext.h"

class Renderer
{
	public:
		Renderer(HWND windowHandle,
				 AthruGPU::GPUMemory& gpuMem,
				 const Microsoft::WRL::ComPtr<ID3D12Device>& device,
				 const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& rndrCmdQueue,
				 const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& rndrCmdList,
				 const Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& rndrCmdAlloc);
		~Renderer();
		void Render(Direct3D* d3d);

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// The intersection shader (traces/marches rays through the scene)
		ComputePass tracer;

		// Bounce preparation shader (material synthesis + export)
		ComputePass bouncePrep;

        // Per-frame lens sampler
        ComputePass lensSampler;

		// Small array of surface sampling shaders; each element performs sampling + shading for a different
		// primitive material (support for diffuse, mirrorlike, refractive, subsurface/snowy, generic subsurface, and furry
		// is expected; only diffuse and mirrorlike are supported at the moment)
		ComputePass bxdfs[6];

		// Small shader for filtering & tonemapping (denoising works well with texture sampling, so that runs
		// directly inside the presentation shader)
		ComputePass post;

		// Useful shaders for indirect dispatch, designed to trim dispatch arguments to match a chosen thread
		// count (128-thread version divides dispatch arguments by [128], 256-thread version divides dispatch arguments
		// by [256], and so on)
		// Defined here instead of globally so I can use rendering-specific resource bindings (avoiding rebinding everything
		// each time I adjust the counter-buffer)
        // Suspect I can work without these if I change counter updates in each shader to map between thread-group sizes instead
        // of naively incrementing the dispatch buffer for each re-emitted ray in a shading pass
		ComputePass dispatchScale512;
		ComputePass dispatchScale256;
		ComputePass dispatchScale128;

		// Path information buffer, updated per-bounce
		// Should update member type appropriately
		AthruGPU::AthruResrc<LiBounce, AthruGPU::RWResrc<AthruGPU::Buffer>> pxPaths;

		// Small buffer letting us restrict path-tracing dispatches to paths persisting after
		// each tracing/processing/sampling iteration
		// Each element is a pixel index
		AthruGPU::AthruResrc<DirectX::XMUINT2, AthruGPU::AppBuffer> traceables;

		// Surface intersection buffer (carries successful intersections across for next-event-estimation + material synthesis)
		// Each element is a pixel index
		AthruGPU::AthruResrc<DirectX::XMUINT2, AthruGPU::AppBuffer> surfIsections;

		// Material intersection buffers
		// Each element is a pixel index
		AthruGPU::AthruResrc<DirectX::XMUINT2, AthruGPU::AppBuffer> diffuIsections;
		AthruGPU::AthruResrc<DirectX::XMUINT2, AthruGPU::AppBuffer> mirroIsections;
		AthruGPU::AthruResrc<DirectX::XMUINT2, AthruGPU::AppBuffer> refraIsections;
		AthruGPU::AthruResrc<DirectX::XMUINT2, AthruGPU::AppBuffer> snowwIsections;
		AthruGPU::AthruResrc<DirectX::XMUINT2, AthruGPU::AppBuffer> ssurfIsections;
		AthruGPU::AthruResrc<DirectX::XMUINT2, AthruGPU::AppBuffer> furryIsections;

		// Counter buffers for indirect dispatch; each buffer contains three 4-byte
		// elements to match the three dispatch axes expected by D3D12
		AthruGPU::AthruResrc<u4Byte,
							 AthruGPU::RWResrc<AthruGPU::Buffer>> ctrs[8];

		// Anti-aliasing integration buffer, allows jittered samples to slowly integrate
		// into coherent images over time
		AthruGPU::AthruResrc<PixHistory,
							 AthruGPU::RWResrc<AthruGPU::Buffer>> aaBuffer;

		// Render output texture, copied to the back-buffer each frame
		AthruGPU::AthruResrc<DirectX::XMFLOAT4,
							 AthruGPU::RWResrc<AthruGPU::Texture>> displayTex;

		// Resource context for [this], helps guarantee descriptors are placed in the order expected by
		// the compute shading passes declared above
		AthruGPU::ResrcContext<std::function<void()>, std::function<void()>,
							   std::function<void()>, std::function<void()>,
							   std::function<void()>, std::function<void()>,
							   std::function<void()>, std::function<void()>,
							   std::function<void()>, std::function<void()>,
							   std::function<void()>, std::function<void()>> resrcCtx;

		// References to Athru's main rendering command-queue/command-list/command-allocator
		const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& renderQueue;
		const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& renderCmdList;
		const Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& renderCmdAllocator;

		// Specialized lens-sampling command-list + command-allocator
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> lensList;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> lensAlloc;

		// Specialized path-tracing command-list + command-allocator +
		// command signatures for indirect dispatch
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> ptCmdList;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> ptCmdAllocator;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> ptCmdSig;

		// Constant PT starting/finished fence values
		static constexpr u8Byte PT_STARTING = 16u;
		static constexpr u8Byte PT_ENDED = 32u;
};
