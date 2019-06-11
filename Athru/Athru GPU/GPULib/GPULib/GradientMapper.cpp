#include <cmath>
#include "GPUServiceCentre.h"
#include "GradientMapper.h"

GradientMapper::GradientMapper(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
                               AthruGPU::GPUMemory& gpuMem,
						       const HWND windowHandle) :
			  planetRaster(device,
			  			   windowHandle,
			 		       "PlanetRaster.cso",
						   1, 1, 1)
{
    // Initialize gradient-map inputs
    //rasterInputBuffer.InitCBuf(device,
    //                           gpuMem,
    //                           (address*)rasterInput);
}

GradientMapper::~GradientMapper()
{
    // Try to guarantee that gradient-mapping constants are unmapped on application exit
    //rasterInputBuffer.~AthruResrc();
}

void GradientMapper::RasterPlanets(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& rndrCmds)
{
	// Prepare constant input + dispatch mapping shader for each planet
	for (int i = 0; i < SceneStuff::BODIES_PER_SYSTEM; i += 1)
	{
        // Update gradient-map inputs
        rasterInput->raster = DirectX::XMUINT4(0, AthruGPU::RASTER_ATLAS_VOLUM,
	    								       AthruGPU::RASTER_ATLAS_WIDTH, AthruGPU::RASTER_ATLAS_WIDTH / 2);

		// Cache coarse gradients for the current planet
		u4Byte axisSize = (u4Byte)std::ceil(std::cbrt(AthruGPU::RASTER_CELL_VOLUM / (float)AthruGPU::RASTER_THREADS_PER_CELL));
		rndrCmds->Dispatch(axisSize, axisSize, axisSize);
	}
}

// Push constructions for this class through Athru's custom allocator
void* GradientMapper::operator new(size_t size)
{
	StackAllocator* allocator = AthruCore::Utility::AccessMemory();
	return allocator->AlignedAlloc(size, (uByte)std::alignment_of<GradientMapper>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void GradientMapper::operator delete(void* target)
{
	return;
}
