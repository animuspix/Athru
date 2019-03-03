#pragma once

#include <d3d12.h>
#include "AthruResrc.h"
#include "PixHistory.h"
#include "ComputeShader.h"
#include "ScreenPainter.h"
#include "Camera.h"
#include "LiBounce.h"

class Renderer
{
	public:
		Renderer(HWND windowHandle,
				 const Microsoft::WRL::ComPtr<ID3D12Device>& device,
				 const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& rndrCmdQueue,
				 const Microsoft::WRL::ComPtr<ID3D12CommandList>& rndrCmdList,
				 const Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& rndrCmdAlloc);
		~Renderer();
		void Render(Camera* camera);

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// Lens sampler
		void SampleLens(DirectX::XMVECTOR& cameraPosition,
						DirectX::XMMATRIX& viewMatrix);

		// Ray/scene intersector & volumetric sampler
		void Trace();

		// Bounce preparation; next-event-estimation & material synthesis, also surface sampling
		void Bounce();

		// Filter/tonemap rendered images before denoising/rasterization in [Present]
		void Prepare();

		// Present the scene to the player through the DirectX geometry pipeline
		// (would love a more direct way to do this)
		void Present(ViewFinder* viewFinder);

		// Small array of tracing shaders; [0] contains the lens-sampler (finds outgoing directions,
		// initializes light paths), [1] contains the intersection shader (traces/marches rays through
		// the scene)
		ComputeShader tracers[2];

		// Bounce preparation shader (material synthesis + export)
		ComputeShader bouncePrep;

		// Small array of sampling shaders; each element performs sampling + shading for a different
		// surface type (support for diffuse, mirrorlike, refractive, subsurface/snowy, generic subsurface, and furry
		// is expected; only diffuse and mirrorlike are supported at the moment)
		ComputeShader samplers[6];

		// Small shader for filtering & tonemapping (denoising works well with texture sampling, so that runs
		// directly inside the presentation shader)
		ComputeShader post;

		// Rasterization shader, needed for mostly-compute graphics under DX11 (afaik);
		// also performs denoising
		ScreenPainter screenPainter;

		// Useful shaders for indirect dispatch, designed to trim dispatch arguments to match a chosen thread
		// count (128-thread version divides dispatch arguments by [128], 256-thread version divides dispatch arguments
		// by [256], and so on)
		// Defined here instead of globally so I can use rendering-specific resource bindings (avoiding rebinding everything
		// each time I adjust the counter-buffer)
		ComputeShader dispatchScale512;
		ComputeShader dispatchScale256;
		ComputeShader dispatchScale128;

		// Rendering-specific input struct
		struct RenderInput
		{
			DirectX::XMVECTOR cameraPos; // Camera position in [xyz], [w] is unused
			DirectX::XMMATRIX viewMat; // View matrix
			DirectX::XMMATRIX iViewMat; // Inverse view matrix
			DirectX::XMFLOAT4 bounceInfo; // Maximum number of bounces in [x], number of supported surface BXDFs in [y],
										  // tracing epsilon value in [z]; [w] is unused
			DirectX::XMUINT4 resInfo; // Resolution info carrier; contains app resolution in [xy],
									  // AA sampling rate in [z], and display area in [w]
			DirectX::XMUINT4 tilingInfo; // Tiling info carrier; contains spatial tile counts in [x]/[y] and cumulative tile area in [z] ([w] is unused)
			DirectX::XMUINT4 tileInfo; // Per-tile info carrier; contains tile width/height in [x]/[y] and per-tile area in [z] ([w] is unused)
		};

		// A reference to the rendering-specific input/constant buffer (with layout defined by [RenderInput])
		AthruGPU::AthruResrc<LiBounce,
							 AthruGPU::AppBuffer,
							 AthruGPU::RESRC_COPY_STATES::NUL,
							 AthruGPU::RESRC_CTX::RNDR_OR_GENERIC> renderInputBuffer;

		// Small buffer letting us restrict path-tracing dispatches to paths persisting after
		// each tracing/processing/sampling iteration
		AthruGPU::AthruResrc<LiBounce,
							 AthruGPU::AppBuffer,
							 AthruGPU::RESRC_COPY_STATES::NUL,
							 AthruGPU::RESRC_CTX::RNDR_OR_GENERIC> traceables;

		// Surface intersection buffer (carries successful intersections across for next-event-estimation + material synthesis)
		AthruGPU::AthruResrc<LiBounce,
							 AthruGPU::AppBuffer,
							 AthruGPU::RESRC_COPY_STATES::NUL,
							 AthruGPU::RESRC_CTX::RNDR_OR_GENERIC> surfIsections;

		// Material intersection buffers
		AthruGPU::AthruResrc<LiBounce, AthruGPU::AppBuffer, AthruGPU::RESRC_COPY_STATES::NUL, AthruGPU::RESRC_CTX::RNDR_OR_GENERIC> diffuIsections;
		AthruGPU::AthruResrc<LiBounce, AthruGPU::AppBuffer, AthruGPU::RESRC_COPY_STATES::NUL, AthruGPU::RESRC_CTX::RNDR_OR_GENERIC> mirroIsections;
		AthruGPU::AthruResrc<LiBounce, AthruGPU::AppBuffer, AthruGPU::RESRC_COPY_STATES::NUL, AthruGPU::RESRC_CTX::RNDR_OR_GENERIC> refraIsections;
		AthruGPU::AthruResrc<LiBounce, AthruGPU::AppBuffer, AthruGPU::RESRC_COPY_STATES::NUL, AthruGPU::RESRC_CTX::RNDR_OR_GENERIC> snowwIsections;
		AthruGPU::AthruResrc<LiBounce, AthruGPU::AppBuffer, AthruGPU::RESRC_COPY_STATES::NUL, AthruGPU::RESRC_CTX::RNDR_OR_GENERIC> ssurfIsections;
		AthruGPU::AthruResrc<LiBounce, AthruGPU::AppBuffer, AthruGPU::RESRC_COPY_STATES::NUL, AthruGPU::RESRC_CTX::RNDR_OR_GENERIC> furryIsections;

		// Generic counter buffer, carries dispatch axis sizes per-material in 0-17,
		// generic axis sizes in 18-20, thread count assumed for dispatch axis sizes
		// in [21], and a light bounce counter in [22]
		// Also raw generic append-buffer lengths in [23], and material append-buffer
		// lengths in 24-29
		AthruGPU::AthruResrc<u4Byte,
							 AthruGPU::CtrBuffer,
							 AthruGPU::RESRC_COPY_STATES::NUL,
							 AthruGPU::RESRC_CTX::RNDR_OR_GENERIC> ctrBuf;

		// [rndrCtr] layout referencess
		const u4Byte RNDR_CTR_OFFSET_GENERIC = (GraphicsStuff::NUM_SUPPORTED_SURF_BXDFS * AthruGPU::DISPATCH_ARGS_SIZE);

		// Anti-aliasing integration buffer, allows jittered samples to slowly integrate
		// into coherent images over time
		AthruGPU::AthruResrc<PixHistory,
							 AthruGPU::RWResrc<AthruGPU::Buffer>,
							 AthruGPU::RESRC_COPY_STATES::NUL,
							 AthruGPU::RESRC_CTX::RNDR_OR_GENERIC> aaBuffer;

		// A reference to the presentation-only input/constant buffer (with layout equivalent to [DirectX::XMFLOAT4])
		//AthruGPU::AthruBuffer<DirectX::XMFLOAT4, AthruGPU::CBuffer> displayInputBuffer;

		// References to Athru's main rendering command-queue/command-list/command-allocator, needed for DX12 work submission
		const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& renderQueue;
		const Microsoft::WRL::ComPtr<ID3D12CommandList>& renderCmdList;
		const Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& renderCmdAllocator;

		// References to utility command lists (bundles) used to encapsulate Athru shading stages
		const Microsoft::WRL::ComPtr<ID3D12CommandList>& sampleLensCmdList; // A small bundle capturing commands issued for lens-sampling
		const Microsoft::WRL::ComPtr<ID3D12CommandList>& ptStepCmdList; // A bundle capturing events at each path-tracing step (tracing rays, evaluating local materials on intersection,
																		// computing path changes and tracing the next step)
		const Microsoft::WRL::ComPtr<ID3D12CommandList>& postCmdList; // Another small bundle capturing commands issued for anti-aliasing, tone-mapping, and denoising
};
