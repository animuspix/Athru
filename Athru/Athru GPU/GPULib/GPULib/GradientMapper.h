#pragma once

#include "GPUGlobals.h"

class GradientMapper
{
	public:
		GradientMapper(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
                     AthruGPU::GPUMemory& gpuMem,
                     const HWND windowHandle);
		~GradientMapper();

		// Map planetary gradients for the local system
		// Assumes planetary figures + the rasterization atlas are already on the GPU
		void RasterPlanets(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& rndrCmds);

		// Map plant/animal gradients for the local planet
		// ...will implement when possible...

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// Structure carrying mapping input information
		struct RasterInput
		{
			// Planet to map in [x], buffer length in [y],
			// mapping width in [z], half mapping width in [w]
			DirectX::XMUINT4 raster;
		};
        // + CPU-side mapping point
        RasterInput* rasterInput;
		// + a matching GPU-side constant buffer
		AthruGPU::AthruResrc<RasterInput,
		                     AthruGPU::CBuffer> rasterInputBuffer;

		// Planet mapper
		ComputePass planetRaster;

		// Plant rasterizer
		// ...will implement when possible...

		// Animal rasterizer
		// ...will implement when possible...

		// Atlas containing mapped planetary gradients for the current system +
		// mapped plants/animals for the nearest planet
		AthruGPU::AthruResrc<float,
                             AthruGPU::UploResrc<AthruGPU::RWResrc<AthruGPU::Buffer>>> rasterAtlas;
};
