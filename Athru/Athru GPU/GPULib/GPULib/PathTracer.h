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
			DirectX::XMFLOAT4 deltaTime; // Change in time betweeen frames
			DirectX::XMUINT4 currTimeSecs; // Current time (in seconds)
			DirectX::XMUINT4 traceableCtr; // Number of traceable elements after each pre-processing pass
			DirectX::XMUINT4 numPathPatches; // Number of patches (groups) along each dispatch axis during path-traced rendering (y)
											 // and pre-processing (x)
			DirectX::XMUINT4 maxNumBounces; // Number of bounces for each ray
			DirectX::XMUINT4 numDirGaths; // Number of direct gathers (area light samples) for each ray
			DirectX::XMUINT4 numIndirGaths; // Number of indirect gathers (local samples) for each ray
		};

		// ...And a reference to the buffer we'll need in order
		// to send that input data over to the GPU
		Microsoft::WRL::ComPtr<ID3D11Buffer> shaderInputBuffer;

		// Small buffer letting us restrict path-tracing dispatches to pixels intersecting
		// planets, plants, and/or animals
		Microsoft::WRL::ComPtr<ID3D11Buffer> traceables;

		// A GPU read/write view over the intersection-buffer declared above
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> traceablesAppendView;

		// Small staging buffer, used so we can read the length of [traceables] between
		// the pre-processing/path-tracing stages and avert [Consume] calls as appropriate
		Microsoft::WRL::ComPtr<ID3D11Buffer> numTraceables;

		// Especially heavy frames will require progressive rendering to safely process; cache the
		// number of [traceables] at the start of each multi-frame render here to allow for
		// consistent dispatch sizes in each pass
		fourByteUnsigned currMaxTraceables;

		// Whether or not the path-tracer is processing the initial frame in a multi-frame
		// (progressive) render
		bool initProgPass;

		// Whether or not the first frame for the current session is being rendered;
		// allows us to zero [traceableBuffer]'s counter then and never again
		bool zerothFrame;

		// Anti-aliasing integration buffer, allows jittered samples to slowly integrate
		// into coherent images over time
		Microsoft::WRL::ComPtr<ID3D11Buffer> aaBuffer;

		// A GPU read/write allowed view of the AA integration buffer
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> aaBufferView;
};
