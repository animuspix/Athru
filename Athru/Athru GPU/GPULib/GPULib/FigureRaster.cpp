#include <cmath>
#include "GPUServiceCentre.h"
#include "FigureRaster.h"

FigureRaster::FigureRaster(const Microsoft::WRL::ComPtr<ID3D11Device>& device,
						   HWND windowHandle) :
			  planetRaster(device,
						   windowHandle,
						   L"PlanetRaster.cso"),
			  rasterAtlas(AthruGPU::AthruBuffer<float, AthruGPU::WLimitedBuffer>(device,
																		 nullptr,
																		 AthruGPU::RASTER_ATLAS_VOLUM,
																		 DXGI_FORMAT_R32_FLOAT)),
			  rasterInput(AthruGPU::AthruBuffer<RasterInput, AthruGPU::CBuffer>(device,
																		nullptr))
{}

FigureRaster::~FigureRaster(){}

void FigureRaster::RasterToGPU(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context)
{
	context->CSSetShaderResources(1, 1, rasterAtlas.rView().GetAddressOf());
}

void FigureRaster::RasterPlanets(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context)
{
	// Pass the rasterization shader onto the pipeline for the current frame
	context->CSSetShader(planetRaster.shader.Get(), nullptr, 0);
	context->CSSetUnorderedAccessViews(5, 1, rasterAtlas.rwView().GetAddressOf(), nullptr);

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
		((RasterInput*)shaderInput.pData)->raster = DirectX::XMUINT4(i, AthruGPU::RASTER_ATLAS_VOLUM,
																	 AthruGPU::RASTER_ATLAS_WIDTH, AthruGPU::RASTER_ATLAS_WIDTH / 2);
		context->Unmap(rasterInput.buf.Get(), 0);
		context->CSSetConstantBuffers(0, 1, rasterInput.buf.GetAddressOf());

		// Rasterize the current planet
		u4Byte axisSize = (u4Byte)std::ceil(std::cbrt(AthruGPU::RASTER_CELL_VOLUM / (float)AthruGPU::RASTER_THREADS_PER_CELL));
		context->Dispatch(axisSize, axisSize, axisSize);
	}

	// The raster-atlas won't be passed as read/write-allowed again until we visit another system, so un-bind its unordered-access-view
	// here
	ID3D11UnorderedAccessView* nullUAV = nullptr;
	context->CSSetUnorderedAccessViews(5, 1, &nullUAV, 0);
}

// Push constructions for this class through Athru's custom allocator
void* FigureRaster::operator new(size_t size)
{
	StackAllocator* allocator = AthruCore::Utility::AccessMemory();
	return allocator->AlignedAlloc(size, (uByte)std::alignment_of<FigureRaster>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void FigureRaster::operator delete(void* target)
{
	return;
}
