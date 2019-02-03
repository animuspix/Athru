#pragma once

#include "AppGlobals.h"
#include "AthruBuffer.h"
#include <math.h>

namespace GraphicsStuff
{
	// DirectX setup information
	extern constexpr float SCREEN_FAR = 1.0f;
	extern constexpr float SCREEN_NEAR = 0.1f;
	extern constexpr float VERT_FIELD_OF_VIEW_RADS = MathsStuff::PI * 0.5f;
	// Move these to in-engine/in-game settings when possible
	extern constexpr float HORI_FIELD_OF_VIEW_RADS = 2.0f * 1.0565311988f; // No trig allowed in [constexpr] functions; second factor precomputed from
																		   // [atan(tan(VERT_FIELD_OF_VIEW_RADS * 0.5f) * DISPLAY_ASPECT_RATIO)]
	extern constexpr float FRUSTUM_WIDTH_AT_NEAR = (2 * GraphicsStuff::SCREEN_NEAR); // No trig allowed in [constexpr] functions; second factor precomputed from
																					 // [tan(VERT_FIELD_OF_VIEW_RADS * 0.5f)]
	extern constexpr float FRUSTUM_HEIGHT_AT_NEAR = FRUSTUM_WIDTH_AT_NEAR / DISPLAY_ASPECT_RATIO;

	// Rendering information
	extern constexpr u4Byte SCREEN_RECT_INDEX_COUNT = 6;
	extern constexpr u4Byte MAX_NUM_BOUNCES = 7;
	extern constexpr u4Byte MAX_NUM_SSURF_BOUNCES = 512;
	extern constexpr u4Byte NUM_AA_SAMPLES = 8192;
	extern constexpr u4Byte TILE_WIDTH = 2;
	extern constexpr u4Byte TILE_HEIGHT = 2;
	extern constexpr u4Byte TILE_AREA = TILE_WIDTH * TILE_HEIGHT;
	extern constexpr u4Byte TILING_WIDTH = DISPLAY_WIDTH / TILE_WIDTH;
	extern constexpr u4Byte TILING_HEIGHT = DISPLAY_HEIGHT / TILE_HEIGHT;
	extern constexpr u4Byte TILING_AREA = TILING_WIDTH * TILING_HEIGHT;
	extern constexpr uByte NUM_SUPPORTED_SURF_BXDFS = 6;
	extern constexpr float EPSILON_MIN = 0.0001f;
	extern constexpr float EPSILON_MAX = 0.1f;

	// Enum list of supported primitive materials
	enum class SUPPORTED_SURF_BXDDFS
	{
		DIFFU = 0,
		MIRRO = 1,
		REFRA = 2,
		SNOWW = 3,
		SSURF = 4,
		FURRY = 5
	};
}

namespace AthruGPU
{
	// Number of random seeds to expose to the GPU random number generator
	extern constexpr u4Byte NUM_RAND_SEEDS = 32917504;

	// Dispatch axis generator for full-screen compute passes
	// Should specify as [consteval] after C++20
	constexpr DirectX::XMUINT3 fullscreenDispatchAxes(u4Byte groupWidth)
	{
		return DirectX::XMUINT3((u4Byte)(std::ceil(float(GraphicsStuff::DISPLAY_WIDTH) / groupWidth)),
								(u4Byte)(std::ceil(float(GraphicsStuff::DISPLAY_HEIGHT) / groupWidth)),
								1);
	}

	// Dispatch axis generator for tiled compute passes (assumed shading-specific)
	// Should specify as [consteval] after C++20
	constexpr DirectX::XMUINT3 tiledDispatchAxes(u4Byte groupWidth)
	{
		return DirectX::XMUINT3((u4Byte)(std::ceil(float(GraphicsStuff::TILING_WIDTH) / groupWidth)),
								(u4Byte)(std::ceil(float(GraphicsStuff::TILING_HEIGHT) / groupWidth)),
								1);
	}

	// SDF atlas information
	extern constexpr u4Byte RASTER_ATLAS_WIDTH = 128;
	extern constexpr u4Byte RASTER_ATLAS_HEIGHT = 384; // One row for planets, one row for animals, one row for plants
	extern constexpr u4Byte RASTER_CELL_DEPTH = 128;
	extern constexpr u4Byte RASTER_CELL_VOLUM = RASTER_CELL_DEPTH * RASTER_ATLAS_WIDTH * RASTER_ATLAS_WIDTH;
	extern constexpr u4Byte RASTER_THREADS_PER_CELL = 512;
	extern constexpr u4Byte RASTER_ATLAS_DEPTH = RASTER_CELL_DEPTH * SceneStuff::PLANETS_PER_SYSTEM; // Less than ten plant/animal types per system seems reasonable...
	extern constexpr u4Byte RASTER_ATLAS_VOLUM = RASTER_ATLAS_WIDTH * RASTER_ATLAS_HEIGHT * RASTER_ATLAS_DEPTH;

	// Indirect dispatch helpers
	extern constexpr u4Byte DISPATCH_ARGS_SIZE = 12;

	// Read structure count for a given append/consume buffer
	template<typename BuffContent>
	u4Byte appConsumeCount(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
						   const Microsoft::WRL::ComPtr<ID3D11Device>& device,
						   AthruBuffer<BuffContent, AppBuffer>& buffer)
	{
		AthruBuffer<u4Byte, StgBuffer> buff = AthruGPU::AthruBuffer<u4Byte, StgBuffer>(device,
																					   nullptr,
																					   1,
																					   DXGI_FORMAT_R32_UINT);
		context->CopyStructureCount(buff.buf.Get(), 0, buffer.view().Get());
		D3D11_MAPPED_SUBRESOURCE count;
		context->Map(buff.buf.Get(), 0, D3D11_MAP_READ, 0, &count);
		u4Byte counter = *(u4Byte*)(count.pData);
		context->Unmap(buff.buf.Get(), 0);
		return counter;
	}
}

