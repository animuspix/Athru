#pragma once

#include <d3d11.h>
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
											// pass in [z], number of patches (groups) deployed
											// for path-tracing in [w]
			DirectX::XMFLOAT4 prevNumTraceables; // Number of traceable elements in the previous frame in [x]
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
		Microsoft::WRL::ComPtr<ID3D11Buffer> traceables;

		// A GPU read/write view over the intersection-buffer declared above
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> traceablesAppendView;

		// Small staging buffer, used so we can read the length of [traceables] between
		// the pre-processing/path-tracing stages and avert [Consume] calls as appropriate
		Microsoft::WRL::ComPtr<ID3D11Buffer> numTraceables;

		// Whether or not the first frame for the current session is being rendered;
		// allows us to zero [traceableBuffer]'s counter then and never again
		bool zerothFrame;

		// Anti-aliasing integration buffer, allows jittered samples to slowly integrate
		// into coherent images over time
		Microsoft::WRL::ComPtr<ID3D11Buffer> aaBuffer;

		// A GPU read/write allowed view of the AA integration buffer
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> aaBufferView;
};
