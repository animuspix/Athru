#pragma once

#include <d3d12.h>
#include <functional>
#include "AthruResrc.h"
#include "PixHistory.h"
#include "ComputePass.h"
#include "Camera.h"
#include "LiBounce.h"
#include "PhiloStrm.h"
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

        // Per-frame lens sampler
        ComputePass lensSampler;

		// Small array of surface sampling shaders; each element performs sampling + shading for a different
		// primitive material (support for diffuse, mirrorlike, refractive, subsurface/snowy, generic subsurface, and furry
		// is expected; only diffuse and mirrorlike are supported at the moment)
		ComputePass bxdfs[6];

		// Small shader for filtering & tonemapping (denoising works well with texture sampling, so that runs
		// directly inside the presentation shader)
		ComputePass post;

		// Buffer of rendering data, sections accessed by different projections declared below
		AthruGPU::AthruResrc<uByte, AthruGPU::RWResrc<AthruGPU::Buffer>> rndrBuff;

		// Path information projections, updated per-bounce
		struct float2x3 { DirectX::XMFLOAT3 r0; DirectX::XMFLOAT3 r1; };
		AthruGPU::AthruResrc<float2x3, AthruGPU::RWResrc<AthruGPU::Buffer>> rays;
		AthruGPU::AthruResrc<DirectX::XMFLOAT3, AthruGPU::RWResrc<AthruGPU::Buffer>> rayOris;
		AthruGPU::AthruResrc<DirectX::XMFLOAT3, AthruGPU::RWResrc<AthruGPU::Buffer>> outDirs;
		AthruGPU::AthruResrc<DirectX::XMFLOAT2, AthruGPU::RWResrc<AthruGPU::Buffer>> iors;
		AthruGPU::AthruResrc<u4Byte, AthruGPU::RWResrc<AthruGPU::Buffer>> figIDs;

		// Small projection letting us restrict path-tracing dispatches to paths persisting after
		// each tracing/processing/sampling iteration
		// Each element is a pixel index
		AthruGPU::AthruResrc<u4Byte, AthruGPU::AppBuffer> traceables;

		// Material intersection projections
		// Each element is a pixel index
		AthruGPU::AthruResrc<u4Byte, AthruGPU::AppBuffer> diffuIsections;
		AthruGPU::AthruResrc<u4Byte, AthruGPU::AppBuffer> mirroIsections;
		AthruGPU::AthruResrc<u4Byte, AthruGPU::AppBuffer> refraIsections;
		AthruGPU::AthruResrc<u4Byte, AthruGPU::AppBuffer> snowwIsections;
		AthruGPU::AthruResrc<u4Byte, AthruGPU::AppBuffer> ssurfIsections;
		AthruGPU::AthruResrc<u4Byte, AthruGPU::AppBuffer> furryIsections;

		// Path-tracing RNG state projection; each chunk of state is a separate Philox stream
		// (see [PhiloStrm.h])
		AthruGPU::AthruResrc<PhiloStrm, AthruGPU::RWResrc<AthruGPU::Buffer>> randState;

		// Set of external counters for append/consume projections; zeroth index matches to
		// traceable indices ([traceables]), indices 1-6 match to the intersection
		// buffers declared above (diffuse/mirrorlike/refractive/snowy/generic-subsurface/furry)
		AthruGPU::AthruResrc<u4Byte, AthruGPU::RWResrc<AthruGPU::Buffer>> counters;

		// Small value recording how far counter values are offset into the rendering buffer
		u4Byte counterOffset;

		// Anti-aliasing integration buffer, allows jittered samples to slowly integrate
		// into coherent images over time
		AthruGPU::AthruResrc<PixHistory,
							 AthruGPU::RWResrc<AthruGPU::Buffer>> aaBuffer;

		// Render output texture, copied to the back-buffer each frame
		AthruGPU::AthruResrc<DirectX::XMFLOAT4,
							 AthruGPU::RWResrc<AthruGPU::Texture>> displayTexHDR;
		AthruGPU::AthruResrc<DirectX::XMFLOAT4,
							 AthruGPU::RWResrc<AthruGPU::Texture>> displayTexLDR;

		// References to Athru's main rendering command-queue/command-list/command-allocator
		const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& renderQueue;
		const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& renderCmdList;
		const Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& renderCmdAllocator;

		// Specialized lens-sampling command-list + command-allocator
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> lensList;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> lensAlloc;

		// Specialized path-tracing command-lists + command-allocators
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> traceCmdList;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> traceCmdAllocator;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> surfCmdList;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> surfCmdAllocator;
};
