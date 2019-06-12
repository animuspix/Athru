#pragma once

#include "AppGlobals.h"
#include <DirectXMath.h>
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

	// Each shader gets one megabyte of local memory for bytecode
	extern constexpr u4Byte SHADER_DXIL_ALLOC = 1048576;

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

	// Default Athru texture clear color
	extern constexpr float DEFAULT_TEX_CLEAR_VALUE[4] = { 1.0f, 0.5f, 0.25f, 1.0f };

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
	// Number of random streams to expose to the GPU random number generator for path tracing
	// Not too many values, so all remain resident on GPU each frame
	extern constexpr u4Byte NUM_RAND_PT_STREAMS = GraphicsStuff::DISPLAY_AREA;

	// Number of random streams to expose to the GPU random number generator for physics & ecology
	// Many more of these neeeded for accurate simulation (independant terrain noise, texture noise,
	// plant/animal distributional noise, possibly stochastic fluid simulation...), so most values will
	// be loaded in tiled/reserved memory and segments needed in each frame will be moved across
	// to the GPU on-demand
	extern constexpr u4Byte NUM_RAND_PHYS_ECO_STREAMS = 28917504;

	// Expected maximum onboard GPU memory usage (for resource data) + 2048 bytes of padding
	// Just working with dedicated memory atm, might implement streaming resources for assets when I start loading them in
	extern constexpr u4Byte EXPECTED_ONBOARD_GPU_RESRC_MEM = 800002048;

	// Minimal byte footprint for aligned D3D12 buffer resources
	extern constexpr u4Byte MINIMAL_D3D12_ALIGNED_BUFFER_MEM = 65536;

	// Expected maximum shared GPU memory usage (for resource upload)
	extern constexpr u4Byte EXPECTED_SHARED_GPU_UPLO_MEM = MINIMAL_D3D12_ALIGNED_BUFFER_MEM;

	// Expected maximum shared GPU memory usage (for resource download/readback)
	// Includes (DISPLAY_AREA * 12) bytes worth of screenshot memory (one set of 32-bit RGB values/pixel) + ~44KB of
	// padding so that the full allocation is 65536-byte aligned
	// Screenshots will be exported as PNG using the lodepng library: https://github.com/lvandeve/lodepng
	extern constexpr u4Byte EXPECTED_SHARED_GPU_RDBK_MEM = (GraphicsStuff::DISPLAY_AREA * 12) + 45056;

	// Expected maximum number of shader-visible resource views
	// Likely to grow after I implement physics + ecology systems
	extern constexpr u4Byte EXPECTED_NUM_GPU_SHADER_VIEWS = 70;

	// Byte footprint for constant data in the CPU->GPU message buffer
	extern constexpr u2Byte EXPECTED_GPU_CONSTANT_MEM = 256;

	// GPU/DX12 heap types available in Athru
	enum class HEAP_TYPES
	{
		UPLO,
		GPU_ACCESS_ONLY,
        READBACK
	};

	// Number of surfaces in the swap-chain (for single/double/triple-buffering)
	extern constexpr uByte NUM_SWAPCHAIN_BUFFERS = 3;

	// Dispatch axis generator for full-screen compute passes
	// Should specify as [consteval] after C++20
	constexpr DirectX::XMUINT3 fullscreenDispatchAxes(u4Byte groupWidth)
	{
		return DirectX::XMUINT3((u4Byte)(ceil(float(GraphicsStuff::DISPLAY_WIDTH) / groupWidth)),
								(u4Byte)(ceil(float(GraphicsStuff::DISPLAY_HEIGHT) / groupWidth)),
								1);
	}

	// Dispatch axis generator for tiled compute passes (assumed shading-specific)
	// Should specify as [consteval] after C++20
	constexpr DirectX::XMUINT3 tiledDispatchAxes(u4Byte groupWidth)
	{
		return DirectX::XMUINT3((u4Byte)(ceil(float(GraphicsStuff::TILING_WIDTH) / groupWidth)),
								(u4Byte)(ceil(float(GraphicsStuff::TILING_HEIGHT) / groupWidth)),
								1);
	}

	// SDF atlas information
	extern constexpr u4Byte RASTER_ATLAS_WIDTH = 128;
	extern constexpr u4Byte RASTER_ATLAS_HEIGHT = 384; // One row for planets, one row for animals, one row for plants
	extern constexpr u4Byte RASTER_CELL_DEPTH = 128;
	extern constexpr u4Byte RASTER_CELL_VOLUM = RASTER_CELL_DEPTH * RASTER_ATLAS_WIDTH * RASTER_ATLAS_WIDTH;
	extern constexpr u4Byte RASTER_THREADS_PER_CELL = 512;
	extern constexpr u4Byte RASTER_ATLAS_DEPTH = RASTER_CELL_DEPTH * SceneStuff::BODIES_PER_SYSTEM; // Less than ten plant/animal types per system seems reasonable...
	extern constexpr u4Byte RASTER_ATLAS_VOLUM = RASTER_ATLAS_WIDTH * RASTER_ATLAS_HEIGHT * RASTER_ATLAS_DEPTH;

	// Indirect dispatch helpers
	extern constexpr u4Byte DISPATCH_ARGS_SIZE = 12;
}

