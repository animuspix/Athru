#include <cmath>
#include "GPUServiceCentre.h"
#include "FigureRaster.h"

FigureRaster::FigureRaster(const Microsoft::WRL::ComPtr<ID3D11Device>& device,
						   HWND windowHandle) :
			  planetRaster(device,
						   windowHandle,
						   L"PlanetRaster.cso"),
			  rasterAtlas(AthruBuffer<float, GPGPUStuff::GPURWBuffer>(device,
																	  nullptr,
																	  GPGPUStuff::RASTER_ATLAS_VOLUM,
																	  DXGI_FORMAT_R32_FLOAT)),
			  rasterInput(AthruBuffer<RasterInput, GPGPUStuff::CBuffer>(device,
																		nullptr))
{}

FigureRaster::~FigureRaster(){}

void FigureRaster::RasterToGPU(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context)
{
	context->CSSetUnorderedAccessViews(5, 1, rasterAtlas.view().GetAddressOf(), nullptr);
}

void FigureRaster::RasterPlanets(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context)
{
	// Pass the rasterization shader onto the pipeline for the current frame
	context->CSSetShader(planetRaster.shader.Get(), nullptr, 0);

	// Prepare constant input + dispatch raster shader for each planet
	for (int i = 0; i < SceneStuff::PLANETS_PER_SYSTEM; i += 1)
	{
		// Prepare cbuffer input
		// Map cbuffer contents to a local CPU reference
		D3D11_MAPPED_SUBRESOURCE shaderInput;
		HRESULT result = context->Map(rasterInput.buf.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &shaderInput);
		assert(SUCCEEDED(result));

		// Initialize rasterization info, close CPU buffer connection,
		// pass back to the GPU
		((RasterInput*)shaderInput.pData)->raster = DirectX::XMUINT4(i, GPGPUStuff::RASTER_ATLAS_VOLUM,
																	 GPGPUStuff::RASTER_ATLAS_WIDTH, GPGPUStuff::RASTER_ATLAS_WIDTH / 2);
		context->Unmap(rasterInput.buf.Get(), 0);
		context->CSSetConstantBuffers(0, 1, rasterInput.buf.GetAddressOf());

		// Rasterize the current planet
		fourByteUnsigned axisSize = (fourByteUnsigned)std::ceil(std::cbrt(GPGPUStuff::RASTER_CELL_VOLUM / (float)GPGPUStuff::RASTER_THREADS_PER_CELL));
		context->Dispatch(axisSize, axisSize, axisSize);
	}
}

// Push constructions for this class through Athru's custom allocator
void* FigureRaster::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<FigureRaster>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void FigureRaster::operator delete(void* target)
{
	return;
}
