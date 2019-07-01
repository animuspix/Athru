#pragma once

#include <d3d12.h>
#include <functional>
#include "AthruResrc.h"
#include "PixHistory.h"
#include "ComputePass.h"
#include "Camera.h"
#include "PhiloStrm.h"

class Renderer
{
	public:
		Renderer(HWND windowHandle,
				 AthruGPU::GPUMemory& gpuMem,
				 const Microsoft::WRL::ComPtr<ID3D12Device>& device,
				 const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& rndrCmdQueue);
		~Renderer();
		void Render(Direct3D* d3d);

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// Per-frame lens sampler
		ComputePass lensSampler;

		// The intersection shader (traces/marches rays through the scene)
		ComputePass tracer;

		// The dispatch generator; used to convert ray/material counters to indirect
		// dispatch arguments (assumes 256 threads/group)
		ComputePass dispEditor;

		// Surface sampling megakernel; every individual group is coherent, but batches of groups are
		// deployed against different materials
		ComputePass surfSampler;

		// Small post-processing shader
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

		// External counters for append/consume projections
		// Declared in separate memory for easier packing (counters are 4096-byte aligned)
		// and to avoid creating inaccessible memory within [rndrBuff]
		AthruGPU::AthruResrc<uByte, AthruGPU::RWResrc<AthruGPU::Buffer>> ctrBuff;
		AthruGPU::AthruResrc<u4Byte, AthruGPU::RWResrc<AthruGPU::Buffer>> traceCtr;
		AthruGPU::AthruResrc<u4Byte, AthruGPU::RWResrc<AthruGPU::Buffer>> diffuCtr;
		AthruGPU::AthruResrc<u4Byte, AthruGPU::RWResrc<AthruGPU::Buffer>> mirroCtr;
		AthruGPU::AthruResrc<u4Byte, AthruGPU::RWResrc<AthruGPU::Buffer>> refraCtr;
		AthruGPU::AthruResrc<u4Byte, AthruGPU::RWResrc<AthruGPU::Buffer>> snowwCtr;
		AthruGPU::AthruResrc<u4Byte, AthruGPU::RWResrc<AthruGPU::Buffer>> ssurfCtr;
		AthruGPU::AthruResrc<u4Byte, AthruGPU::RWResrc<AthruGPU::Buffer>> furryCtr;

		// Set of shader-visible tracing/sampling dispatch axes exposed each frame
		// Successful intersections are recorded here for sampling, and recorded again if they
		// pass through sampling without being absorbed by the scene or hitting a light source
		AthruGPU::AthruResrc<u4Byte, AthruGPU::RWResrc<AthruGPU::Buffer>> dispAxesWrite;

		// Argument buffer for indirect tracing + sampling dispatches
		// Axes recorded by [dispAxesWrite] are copied in here before launching tracing/sampling passes;
		// this allows indirectly dispatched passes to emit more indirect threads themselves (like a tracing
		// pass processing persistent rays from a previous sampling pass, while feeding successful intersections
		// into another sampling pass within the same bounce)
		AthruGPU::AthruResrc<DirectX::XMFLOAT3, AthruGPU::IndirArgBuffer> dispAxesArgs;

		// Anti-aliasing integration buffer, allows jittered samples to slowly integrate
		// into coherent images over time
		AthruGPU::AthruResrc<PixHistory,
							 AthruGPU::RWResrc<AthruGPU::Buffer>> aaBuffer;

		// Render output texture, copied to the back-buffer each frame
		// HDR staging texture is full-resolution for now (like the intermediate PT buffers above) but might tile
		// for better memory usage/access in future builds
		AthruGPU::AthruResrc<DirectX::XMFLOAT4,
							 AthruGPU::RWResrc<AthruGPU::Texture>> displayTexHDR;
		AthruGPU::AthruResrc<DirectX::XMFLOAT4,
							 AthruGPU::RWResrc<AthruGPU::Texture>> displayTexLDR;

		// 128x128 tiled blue-noise dither texture for lens sampling; separates LDS iterations for different pixels
		// to allow convergent sampling without sacrificing parallelism
		// Part of a collection of blue-noise textures licensed to the public domain by Moments In Graphics; the original release
		// is available here :
		// http://momentsingraphics.de/?p=127
		AthruGPU::AthruResrc<DirectX::XMFLOAT2,
							 AthruGPU::RResrc<AthruGPU::Texture>> ditherTex;

		// A screenshot buffer, updated from the LDR display texture per-frame
		AthruGPU::AthruResrc<uByte4,
							 AthruGPU::RWResrc<AthruGPU::Buffer>> displayTexExportable;

		// Position + normal buffers for edge-avoiding wavelet denoise
		// (from http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.672.3796&rep=rep1&type=pdf and
		//	https://www.shadertoy.com/view/ldKBzG)
		AthruGPU::AthruResrc<DirectX::XMFLOAT3,
							 AthruGPU::RWResrc<AthruGPU::Buffer>> denoisePositions;
		AthruGPU::AthruResrc<DirectX::XMFLOAT3,
							 AthruGPU::RWResrc<AthruGPU::Buffer>> denoiseNormals;

		// Reference to Athru's main rendering command-queue
		const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& rnderQueue;

		// A shared rendering command-allocator
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> rnderAlloc;

		// Specialized lens-sampling command-list
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> lensList;

		// Specialized path-tracing command-list
		// (+ command signature for indirect dispatch)
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> ptCmdList;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> ptCmdSignature;

		// Specialized post-processing command-lists
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> postCmdLists[AthruGPU::NUM_SWAPCHAIN_BUFFERS]; // One command-list for each swap-chain buffer

		// Array of command-list sets to fire for each swap-chain buffer
		// (batched version of lensList/ptCmdList/postCmdList[0...2] for neater command submission)
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> rnderCmdSets[3][AthruGPU::NUM_SWAPCHAIN_BUFFERS];

		// Screenshot command list
		// (isolated from main rendering work since screenshot copy-out to readback memory may/may-not happen per-frame)
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> screenShotCmds;

		// Not every frame is a rendering frame, so internally keep track of render frames here
		u4Byte rnderFrameCtr;
};
