#pragma once

#include <chrono>
#include <math.h>
#include <assert.h>
#include <d3d11.h>
#include "Typedefs.h"
#include <wrl\client.h>

namespace TimeStuff
{
	// Non-constant globals initialized in [AthruGlobals.cpp]
	extern std::chrono::high_resolution_clock::time_point timeAtLastFrame;
	extern fourByteUnsigned frameCtr;

	// [inline] instead of a macro so we get type-safety :)
	inline double deltaTime()
	{
		std::chrono::high_resolution_clock::time_point current = std::chrono::steady_clock::now();
		std::chrono::duration<double, std::milli> deltaTimeValue = (current - timeAtLastFrame);
		return deltaTimeValue.count();
	}

	inline double FPS()
	{
		return std::round(1.0f / deltaTime());
	}
}

namespace MathsStuff
{
	// An approximation of pi
	extern constexpr float PI = 3.14159265359f;
}

namespace MemoryStuff
{
	// Type to use for memory-marker indices
	typedef byteUnsigned MARKER_INDEX_TYPE;

	// Maximum supported number of memory-markers
	constexpr byteUnsigned MAX_MARKER_COUNT = 120;

	// Small compile-time function returning whether or
	// not the current target platform is 64-bit
	constexpr bool platform64()
	{
		#ifdef _WIN64
			return true; // The current platform is 64-bit
		#else
			return false; // The current platform is 32-bit
		#endif
	}

	// Address length for the current target platform
	constexpr byteUnsigned addrLength()
	{
		byteUnsigned addressLen = 32; // Assume 32-bit by default
		if constexpr (platform64())
		{
			addressLen = 64; // Update for 64-bit builds
		}
		return addressLen; // Return the evaluated address size
	}

	// Half-address length for the current target platform, required
	// for transforming 64-bit pointers into two-vectors of 32-bit
	// values (needed if we want to easily map changes applied to
	// data on the GPU back to [SceneFigure]s in CPU-accessible
	// memory)
	constexpr byteUnsigned halfAddrLength()
	{
		return addrLength() / 2;
	}

	// Type matching the pointer-length described above; useful
	// for manipulating pointers as natural numbers (e.g. when
	// adjusting memory alignment, fitting 64-bit addresses into
	// 32-bit vectors for transport to the GPU, etc.)
	// std::conditional takes a boolean value argument and two
	// type arguments; the [type] member compiles into the first
	// type argument if the boolean value is [true], and compiles
	// into the second type argument if the boolean value is
	// [false]
	typedef std::conditional<platform64(),
							 eightByteUnsigned,
							 fourByteUnsigned>::type addrValType;

	// Appropriately-sized LO bitmask for the current target
	// platform
	// Required for matching appropriate parts of input pointers
	// to the X and Y axes of storage vectors during CPU->GPU
	// pointer transformations
	constexpr addrValType addrLOMask()
	{
		addrValType mask = 0x0000FFFF; // Assume 32-bit by default
		if constexpr (platform64()) // Adjust for 64-bit builds
		{
			mask = 0x00000000FFFFFFFF;
		}
		return mask; // Return the generated bit-mask
	}

	// Appropriately-sized HI bitmask for the current target
	// platform
	// Required for matching appropriate parts of input pointers
	// to the X and Y axes of storage vectors during CPU->GPU
	// pointer transformations
	constexpr addrValType addrHIMask()
	{
		addrValType mask = 0xFFFF0000; // Assume 32-bit by default
		if constexpr (platform64()) // Adjust for 64-bit builds
		{
			mask = 0xFFFFFFFF00000000;
		}
		return mask; // Return the generated bit-mask
	}

	// Small global function to allocate an arbitrary-type array
	// within the memory controlled by StackAllocator(...)
	template <typename arrayType>
	static arrayType* ArrayAlloc(eightByteUnsigned length,
								 bool setMarker)
	{
		return (arrayType*)AthruUtilities::UtilityServiceCentre::AccessMemory()->AlignedAlloc(length * sizeof(arrayType),
																							  std::alignment_of<arrayType>(),
																							  setMarker);
	}
}

namespace GraphicsStuff
{
	// Display properties
	extern constexpr bool FULL_SCREEN = false;
	extern constexpr bool VSYNC_ENABLED = false;
	extern constexpr fourByteUnsigned DISPLAY_WIDTH = 1920;
	extern constexpr fourByteUnsigned DISPLAY_HEIGHT = 1080;
	extern constexpr fourByteUnsigned DISPLAY_AREA = DISPLAY_WIDTH * DISPLAY_HEIGHT;
	extern constexpr float DISPLAY_ASPECT_RATIO = (float)GraphicsStuff::DISPLAY_WIDTH /
												  (float)GraphicsStuff::DISPLAY_HEIGHT;
	extern constexpr float SCREEN_FAR = 1000.0f;
	extern constexpr float SCREEN_NEAR = 0.1f;
	extern const float FRUSTUM_WIDTH_AT_NEAR;
	extern const float FRUSTUM_HEIGHT_AT_NEAR;
	extern const float VERT_FIELD_OF_VIEW_RADS;
	extern const float HORI_FIELD_OF_VIEW_RADS;

	// Rendering information
	extern constexpr fourByteUnsigned SCREEN_RECT_INDEX_COUNT = 6;
	extern constexpr fourByteUnsigned GROUP_WIDTH_PATH_REDUCTION = 16;
	extern constexpr fourByteUnsigned GROUP_HEIGHT_PATH_REDUCTION = 16;
	extern constexpr fourByteUnsigned GROUP_AREA_PATH_REDUCTION = 256;
	extern constexpr fourByteUnsigned MAX_NUM_BOUNCES = 7;
	extern constexpr fourByteUnsigned NUM_AA_SAMPLES = 8192;
}

namespace SceneStuff
{
	extern constexpr fourByteUnsigned SYSTEM_COUNT = 100;
	extern constexpr fourByteUnsigned PLANETS_PER_SYSTEM = 10;
	extern constexpr fourByteUnsigned PLANTS_PER_PLANET = 10;
	extern constexpr fourByteUnsigned ANIMALS_PER_PLANET = 10;
	extern constexpr fourByteUnsigned MAX_NUM_SCENE_FIGURES = PLANETS_PER_SYSTEM +
															  PLANTS_PER_PLANET +
															  ANIMALS_PER_PLANET;
	extern constexpr fourByteUnsigned MAX_NUM_SCENE_ORGANISMS = 8;
}

namespace GPGPUStuff
{
	// Number of random seeds to expose to the GPU random number generator
	extern constexpr fourByteUnsigned NUM_RAND_SEEDS = 32917504;

	// SDF atlas information
	extern constexpr fourByteUnsigned RASTER_ATLAS_WIDTH = 128;
	extern constexpr fourByteUnsigned RASTER_ATLAS_HEIGHT = 384; // One row for planets, one row for animals, one row for plants
	extern constexpr fourByteUnsigned RASTER_CELL_DEPTH = 128;
	extern constexpr fourByteUnsigned RASTER_CELL_VOLUM = RASTER_CELL_DEPTH * RASTER_ATLAS_WIDTH * RASTER_ATLAS_WIDTH;
	extern constexpr fourByteUnsigned RASTER_THREADS_PER_CELL = 512;
	extern constexpr fourByteUnsigned RASTER_ATLAS_DEPTH = RASTER_CELL_DEPTH * SceneStuff::PLANETS_PER_SYSTEM; // Less than ten plant/animal types per system seems reasonable...
	extern constexpr fourByteUnsigned RASTER_ATLAS_VOLUM = RASTER_ATLAS_WIDTH * RASTER_ATLAS_HEIGHT * RASTER_ATLAS_DEPTH;

	// Supported buffer types in Athru
	struct CBuffer {}; // Constant buffer, read/writable from the CPU/GPU
	struct GPURWBuffer {}; // Generic UAV buffer, read/writable from the GPU but not the CPU
	struct AppBuffer : GPURWBuffer {}; // Generic UAV buffer with the same read/write qualities as [GPURWBuffer], but behaves
									   // like a parallel stack instead of an array (insertion with [Append()], removal with
									   // [Consume()])
	struct GPURBuffer {};  // Generic SRV buffer, readable but not writable from the GPU, inaccessible from the CPU
	struct StrmBuffer {}; // Streaming buffer with CPU-write permissions but not CPU-read; GPU read-only
	struct StgBuffer {}; // Staging buffer, used to copy GPU-only information across to the CPU (useful for e.g.
					     // extracting structure counts from append/consume buffers)
}