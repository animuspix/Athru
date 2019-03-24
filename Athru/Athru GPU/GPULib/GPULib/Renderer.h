#pragma once

#include <d3d12.h>
#include "AthruResrc.h"
#include "PixHistory.h"
#include "ComputePass.h"
#include "Camera.h"
#include "LiBounce.h"

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

		// Small array of sampling shaders; each element performs sampling + shading for a different
		// surface type (support for diffuse, mirrorlike, refractive, subsurface/snowy, generic subsurface, and furry
		// is expected; only diffuse and mirrorlike are supported at the moment)
		// Although these are technically surface samplers, the first index contains a specialty lens sampler to generate
		// initial ray positions and directions in each frame
		ComputePass samplers[7];

		// Small shader for filtering & tonemapping (denoising works well with texture sampling, so that runs
		// directly inside the presentation shader)
		ComputePass post;

		// Useful shaders for indirect dispatch, designed to trim dispatch arguments to match a chosen thread
		// count (128-thread version divides dispatch arguments by [128], 256-thread version divides dispatch arguments
		// by [256], and so on)
		// Defined here instead of globally so I can use rendering-specific resource bindings (avoiding rebinding everything
		// each time I adjust the counter-buffer)
		ComputePass dispatchScale512;
		ComputePass dispatchScale256;
		ComputePass dispatchScale128;

		// Small buffer letting us restrict path-tracing dispatches to paths persisting after
		// each tracing/processing/sampling iteration
		AthruGPU::AthruResrc<LiBounce, AthruGPU::AppBuffer> traceables;

		// Surface intersection buffer (carries successful intersections across for next-event-estimation + material synthesis)
		AthruGPU::AthruResrc<LiBounce, AthruGPU::AppBuffer> surfIsections;

		// Material intersection buffers
		AthruGPU::AthruResrc<LiBounce, AthruGPU::AppBuffer> diffuIsections;
		AthruGPU::AthruResrc<LiBounce, AthruGPU::AppBuffer> mirroIsections;
		AthruGPU::AthruResrc<LiBounce, AthruGPU::AppBuffer> refraIsections;
		AthruGPU::AthruResrc<LiBounce, AthruGPU::AppBuffer> snowwIsections;
		AthruGPU::AthruResrc<LiBounce, AthruGPU::AppBuffer> ssurfIsections;
		AthruGPU::AthruResrc<LiBounce, AthruGPU::AppBuffer> furryIsections;

		// Generic counter buffer, carries dispatch axis sizes per-material in 0-17,
		// generic axis sizes in 18-20, thread count assumed for dispatch axis sizes
		// in [21], and a light bounce counter in [22]
		// Also raw generic append-buffer lengths in [23], and material append-buffer
		// lengths in 24-29
		AthruGPU::AthruResrc<u4Byte,
							 AthruGPU::RWResrc<AthruGPU::Buffer>> ctrBuf;

		// [rndrCtr] layout referencess
		const u4Byte RNDR_CTR_OFFSET_GENERIC = (GraphicsStuff::NUM_SUPPORTED_SURF_BXDFS * AthruGPU::DISPATCH_ARGS_SIZE);

		// Anti-aliasing integration buffer, allows jittered samples to slowly integrate
		// into coherent images over time
		AthruGPU::AthruResrc<PixHistory,
							 AthruGPU::RWResrc<AthruGPU::Buffer>> aaBuffer;

		// Render output texture, copied to the back-buffer each frame
		AthruGPU::AthruResrc<DirectX::XMFLOAT4,
							 AthruGPU::RWResrc<AthruGPU::Texture>> displayTex;

		// References to Athru's main rendering command-queue/command-list/command-allocator, needed for DX12 work submission
		const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& renderQueue;
		const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& renderCmdList;
		const Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& renderCmdAllocator;

		// Utility command lists (bundles) used to encapsulate Athru shading stages
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> sampleLens; // A small bundle capturing commands issued for lens-sampling
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> ptStep; // A bundle capturing events at each path-tracing step (tracing rays, evaluating local materials on intersection,
																  // computing path changes and tracing the next step)
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> postProc; // Another small bundle capturing commands issued for anti-aliasing, tone-mapping, and denoising

		// Utility command allocators; each allocator captures state for one of the bundles described above
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> lensCmds;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> ptStepCmds;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> postProcCmds;
};
