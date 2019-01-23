#pragma once

#include "AppGlobals.h"

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
	extern constexpr u4Byte GROUP_WIDTH_PATH_REDUCTION = 16;
	extern constexpr u4Byte GROUP_HEIGHT_PATH_REDUCTION = 16;
	extern constexpr u4Byte GROUP_AREA_PATH_REDUCTION = 256;
	extern constexpr u4Byte MAX_NUM_BOUNCES = 7;
	extern constexpr u4Byte MAX_NUM_SSURF_BOUNCES = 512;
	extern constexpr u4Byte NUM_AA_SAMPLES = 8192;
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

namespace GPGPUStuff
{
	// Number of random seeds to expose to the GPU random number generator
	extern constexpr u4Byte NUM_RAND_SEEDS = 32917504;

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

	// Supported buffer types in Athru
	struct CBuffer {}; // Constant buffer, read/writable from the CPU/GPU
	struct GPURWBuffer {}; // Generic UAV buffer, read/writable from the GPU but not the CPU
	struct AppBuffer : GPURWBuffer {}; // Generic UAV buffer with the same read/write qualities as [GPURWBuffer], but behaves
									   // like a parallel stack instead of an array (insertion with [Append()], removal with
									   // [Consume()])
	struct WLimitedBuffer : GPURWBuffer {}; // Same behaviour as GPURWBuffer, GPU writability can be toggled on/off
	struct GPURBuffer {}; // Generic SRV buffer, readable but not writable from the GPU, inaccessible from the CPU
	struct StrmBuffer {}; // Streaming buffer with CPU-write permissions but not CPU-read; GPU read-only
	struct StgBuffer {}; // Staging buffer, used to copy GPU-only information across to the CPU (useful for e.g.
						 // extracting structure counts from append/consume buffers)
	struct CtrBuffer : GPURWBuffer {}; // Counter buffer used to streamline compute-shader dispatch; contains at least one set of three elements
									   // representing a dispatch vector
}

