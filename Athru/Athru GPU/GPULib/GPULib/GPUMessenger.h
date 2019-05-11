#pragma once

#include <d3d12.h>
#include <functional>
#include "SceneFigure.h"
#include "AthruResrc.h"
#include "ComputePass.h"
#include <wrl\client.h>
#include "ResrcContext.h"

class GPUMessenger
{
	public:
		GPUMessenger(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
                     AthruGPU::GPUMemory& gpuMem);
		~GPUMessenger();

		// Upload per-system scene data to the GPU
        // Per-planet upload is unimplemented atm, but expected once I have planetary UVs ready
		void SysToGPU(SceneFigure::Figure* system);

		// Pass per-frame inputs along to the GPU
		void InputsToGPU(const DirectX::XMFLOAT4& sysOri,
                         const Camera* camera);

		// Generate + return a lambda wrapping the render input buffer's initializer so it can be placed
		// appropriately inside the rendering section of the global cbv/srv/uav descriptor heap
		std::function<void(const Microsoft::WRL::ComPtr<ID3D12Device>&, AthruGPU::GPUMemory&)> RenderInputInitter();

		// Generic/utility resources might be allocated from outside [GPUMessenger], so make the generic resource context
		// available here
		AthruGPU::ResrcContext<std::function<void()>, std::function<void()>>& AccessResrcContext();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// Generic constant inputs
		struct GPUInput
		{
			DirectX::XMFLOAT4 tInfo; // Time info for each frame;
									 // delta-time in [x], current total time (seconds) in [y],
									 // frame count in [z], nothing in [w]
			DirectX::XMFLOAT4 systemOri; // Origin for the current star-system; useful for predicting figure
										 // positions during ray-marching/tracing (in [xyz], [w] is unused)
		};
        // + CPU-side mapping point
        GPUInput* gpuInput;
		// + a matching GPU-side constant buffer
		AthruGPU::AthruResrc<GPUInput,
		                     AthruGPU::CBuffer,
		                     AthruGPU::RESRC_COPY_STATES::NUL> gpuInputBuffer;

        // Path-tracing inputs
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
        // + CPU side mapping point
        RenderInput* rndrInput;
		// + a matching GPU-side constant buffer
		AthruGPU::AthruResrc<RenderInput,
		                     AthruGPU::CBuffer,
		                     AthruGPU::RESRC_COPY_STATES::NUL> renderInputBuffer;

		// System buffer
        AthruGPU::AthruResrc<SceneFigure::Figure,
                             AthruGPU::UploResrc<AthruGPU::RWResrc<AthruGPU::Buffer>>,
                             AthruGPU::RESRC_COPY_STATES::NUL> sysBuf;

		// Generic/utility shading context
		AthruGPU::ResrcContext<std::function<void()>, std::function<void()>> resrcContext;
};