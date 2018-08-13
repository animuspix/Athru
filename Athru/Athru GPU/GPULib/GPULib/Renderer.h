#pragma once

#include <d3d11.h>
#include "AthruBuffer.h"
#include "PixHistory.h"
#include "ComputeShader.h"
#include "ScreenPainter.h"
#include "Camera.h"

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
		// Private utility function to pre-process the scene for path-tracing
		void PreProcess(DirectX::XMVECTOR& cameraPosition,
						DirectX::XMMATRIX& viewMatrix,
						Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& displayTexWritable);

		// Private utility function to initiate path-tracing
		void Trace(DirectX::XMVECTOR& cameraPosition,
				   DirectX::XMMATRIX& viewMatrix);

		// Private utility function to present the scene to the player through the DirectX
		// geometry pipeline (would love a more direct way to do this)
		void Present(ViewFinder* viewFinder);

		// Small struct to let us access the [shader] member in each local compute shader
		// (so we can pass it into the pipeline) without neccessarily making all [shader]
		// members public in all shader wrapper objects (like e.g. [ScreenPainter])
		struct LocalComputeShader : ComputeShader
		{
			public:
				LocalComputeShader(const Microsoft::WRL::ComPtr<ID3D11Device>& device,
								   HWND windowHandle,
								   LPCWSTR shaderFilePath) : ComputeShader(device,
																		   windowHandle,
																		   shaderFilePath) {}
				const Microsoft::WRL::ComPtr<ID3D11ComputeShader>& d3dShader() { return shader; }
		};

		// Small array of compute shaders; [0] contains the scene-visualization shader (performs BPDT with PSR),
		// [1] contains the path-reduction shader (instantly shades non-intersecting pixels/pixels that immediately
		// intersect the local star, only passes guaranteed-intersecting pixels along for full bi-directional path
		// tracing)
		LocalComputeShader tracers[2];

		// A simple input struct that provides various per-frame
		// constants to the GPU
		struct InputStuffs
		{
			DirectX::XMVECTOR cameraPos; // Camera position
			DirectX::XMMATRIX viewMat; // View matrix
			DirectX::XMMATRIX iViewMat; // Inverse view matrix
			DirectX::XMFLOAT4 timeDispInfo; // Time/dispatch info for each frame;
											// delta-time in [x], current total time (seconds) in [y],
											// number of traceable elements after each pre-processing
											// pass in [z], number of patches (groups) per-axis
											// deployed for path tracing in [w]
			DirectX::XMFLOAT4 prevNumTraceables; // Maximum number of traceable elements in the previous frame in [x]
												 // ([yzw] are empty)
			DirectX::XMUINT4 maxNumBounces; // Number of bounces for each ray
		};

		// ...And a reference to the buffer we'll need in order
		// to send that input data over to the GPU
		AthruBuffer<InputStuffs, GPGPUStuff::CBuffer> shaderInputBuffer;

		// Frame counter for managing time-dependant input values (like [prevNumTraceables])
		fourByteSigned prevNumTraceables;

		// Small buffer letting us restrict path-tracing dispatches to pixels intersecting
		// planets, plants, and/or animals
		// (excluding pixels culled by SWR)
		struct float2x3 { DirectX::XMFLOAT3 rows[2]; };
		AthruBuffer<float2x3, GPGPUStuff::AppBuffer> traceables;

		// Small staging buffer, used so we can read the length of [traceables] after
		// the pre-processing stage and deploy path-tracing threads appropriately
		AthruBuffer<fourByteUnsigned, GPGPUStuff::StgBuffer> numTraceables;

		// Secondary buffer counting the maximum number of intersections in each
		// path-reduction pass (including pixels culled by SWR)
		AthruBuffer<fourByteSigned, GPGPUStuff::GPURWBuffer> maxTraceablesCtr;

		// Staging buffer providing CPU access to [traceableCtr]
		AthruBuffer<fourByteSigned, GPGPUStuff::StgBuffer> maxTraceablesCtrCPU;

		// Anti-aliasing integration buffer, allows jittered samples to slowly integrate
		// into coherent images over time
		AthruBuffer<PixHistory, GPGPUStuff::GPURWBuffer> aaBuffer;

		// Reference to the Direct3D device context
		const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context;

		// Reference to the pipeline shader needed to denoise traced imagery before
		// passing it along to the DX11 render target
		ScreenPainter screenPainter;
};