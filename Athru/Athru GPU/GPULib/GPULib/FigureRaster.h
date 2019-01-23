#pragma once

#include "GPUGlobals.h"

class FigureRaster
{
	public:
		FigureRaster(const Microsoft::WRL::ComPtr<ID3D11Device>& device, HWND windowHandle);
		~FigureRaster();

		// Pass the rasterization atlas to the GPU
		void RasterToGPU(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context);

		// Rasterize system planets
		// Assumes planetary figures + the rasterization atlas are already on the GPU
		void RasterPlanets(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context);

		// Rasterize planet plants/animals
		// ...will implement when possible...

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// Structure carrying rasterization input information
		struct RasterInput
		{
			// Planet to rasterize in [x], buffer length in [y],
			// rasterization width in [z], half rasterization width in [w]
			DirectX::XMUINT4 raster;
		};

		// GPU buffer exposing [RasterInput] to the rasterization shader
		AthruBuffer<RasterInput, GPGPUStuff::CBuffer> rasterInput;

		// Planet rasterizer
		ComputeShader planetRaster;

		// Plant rasterizer
		// ...will implement when possible...

		// Animal rasterizer
		// ...will implement when possible...

		// Atlas containing rasterized planet SDFs for the current system +
		// rasterized plants/animals for the nearest planet
		AthruBuffer<float, GPGPUStuff::WLimitedBuffer> rasterAtlas;
};

