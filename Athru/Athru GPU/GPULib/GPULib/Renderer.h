#pragma once

#include <d3d11.h>
#include "AthruBuffer.h"
#include "PixHistory.h"
#include "ComputeShader.h"
#include "ScreenPainter.h"
#include "Camera.h"
#include "LiBounce.h"

class Renderer
{
	public:
		Renderer(HWND windowHandle,
				 const Microsoft::WRL::ComPtr<ID3D11Device>& device,
				 const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& d3dContext);
		~Renderer();
		void Render(Camera* camera);

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// Lens sampler
		void SampleLens(DirectX::XMVECTOR& cameraPosition,
						DirectX::XMMATRIX& viewMatrix,
						const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& displayTexRW);

		// Ray/scene intersector & volumetric sampler
		void Trace();

		// Bounce preparation; next-event-estimation & material synthesis, also surface sampling
		void Bounce(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& displayTexRW);

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
		};

		// A reference to the rendering-specific input/constant buffer (with layout defined by [RenderInput])
		AthruGPU::AthruBuffer<RenderInput, AthruGPU::CBuffer> renderInputBuffer;

		// Small buffer letting us restrict path-tracing dispatches to paths persisting after
		// each tracing/processing/sampling iteration
		AthruGPU::AthruBuffer<LiBounce, AthruGPU::AppBuffer> traceables;

		// Surface intersection buffer (carries successful intersections across for next-event-estimation + material synthesis)
		AthruGPU::AthruBuffer<LiBounce, AthruGPU::AppBuffer> surfIsections;

		// Material intersection buffers
		AthruGPU::AthruBuffer<LiBounce, AthruGPU::AppBuffer> diffuIsections;
		AthruGPU::AthruBuffer<LiBounce, AthruGPU::AppBuffer> mirroIsections;
		AthruGPU::AthruBuffer<LiBounce, AthruGPU::AppBuffer> refraIsections;
		AthruGPU::AthruBuffer<LiBounce, AthruGPU::AppBuffer> snowwIsections;
		AthruGPU::AthruBuffer<LiBounce, AthruGPU::AppBuffer> ssurfIsections;
		AthruGPU::AthruBuffer<LiBounce, AthruGPU::AppBuffer> furryIsections;

		// Generic counter buffer, carries dispatch axis sizes per-material in 0-17,
		// generic axis sizes in 18-20, thread count assumed for dispatch axis sizes
		// in [21], and a light bounce counter in [22]
		// Also raw generic append-buffer lengths in [23], and material append-buffer
		// lengths in 24-29
		AthruGPU::AthruBuffer<u4Byte, AthruGPU::CtrBuffer> ctrBuf;

		// [rndrCtr] layout referencess
		const u4Byte RNDR_CTR_OFFSET_GENERIC = (GraphicsStuff::NUM_SUPPORTED_SURF_BXDFS * AthruGPU::DISPATCH_ARGS_SIZE);

		// Anti-aliasing integration buffer, allows jittered samples to slowly integrate
		// into coherent images over time
		AthruGPU::AthruBuffer<PixHistory, AthruGPU::GPURWBuffer> aaBuffer;

		// A reference to the presentation-only input/constant buffer (with layout equivalent to [DirectX::XMFLOAT4])
		AthruGPU::AthruBuffer<DirectX::XMFLOAT4, AthruGPU::CBuffer> displayInputBuffer;

		// Reference to the Direct3D device context
		const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context;
};
