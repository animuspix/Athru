#pragma once

#include <d3d11.h>
#include "AthruBuffer.h"
#include "ComputeShader.h"

class PathTracer
{
	public:
		PathTracer(LPCWSTR sceneVisFilePath,
				   LPCWSTR pathReduceFilePath,
				   HWND windowHandle,
				   const Microsoft::WRL::ComPtr<ID3D11Device>& device);
		~PathTracer();
		void PreProcess(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
						const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& displayTexWritable,
						DirectX::XMVECTOR& cameraPosition,
						DirectX::XMMATRIX& viewMatrix);
		void Dispatch(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
					  DirectX::XMVECTOR& cameraPosition,
					  DirectX::XMMATRIX& viewMatrix);

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
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
		Microsoft::WRL::ComPtr<ID3D11Buffer> shaderInputBuffer;

		// Frame counter for managing time-dependant input values (like [prevNumTraceables])
		fourByteUnsigned prevNumTraceables;

		// Small buffer letting us restrict path-tracing dispatches to pixels intersecting
		// planets, plants, and/or animals
		// (excluding pixels culled by SWR)
		Microsoft::WRL::ComPtr<ID3D11Buffer> traceables;

		// A GPU read/write view over the intersection-buffer declared above
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> traceablesAppendView;

		// Small staging buffer, used so we can read the length of [traceables] after
		// the pre-processing stage and deploy path-tracing threads appropriately
		Microsoft::WRL::ComPtr<ID3D11Buffer> numTraceables;

		// Secondary buffer defining the maximum number of intersections in each
		// path-reduction pass (including pixels culled by SWR)
		Microsoft::WRL::ComPtr<ID3D11Buffer> maxTraceables;

		// A GPU read/write view over the data stored in [maxTraceables]
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> maxTraceablesAppendView;

		// Secondary staging buffer carrying the maximum number of traceables in each frame
		// before stochastic work reduction; used so we can calculate an appropriate culling
		// frequency per-frame without referencing the most recent (culled) value in
		// [numTraceables]
		Microsoft::WRL::ComPtr<ID3D11Buffer> maxNumTraceables;

		// Anti-aliasing integration buffer, allows jittered samples to slowly integrate
		// into coherent images over time
		Microsoft::WRL::ComPtr<ID3D11Buffer> aaBuffer;

		// A GPU read/write allowed view of the AA integration buffer
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> aaBufferView;
};
