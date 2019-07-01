#pragma once

#include <chrono>
#include <math.h>
#include <assert.h>
#include <d3d12.h>
#include "Typedefs.h"
#include <wrl\client.h>

namespace TimeStuff
{
	// Non-constant globals initialized in [AthruGlobals.cpp]
	extern std::chrono::high_resolution_clock::time_point lastFrameTime;
	extern u4Byte frameCtr;
	// Constant globals
	constexpr double nsToSecs = 1.0 / 1000000000.0;

	inline float deltaTime()
	{
		std::chrono::high_resolution_clock::time_point t = std::chrono::steady_clock::now();
		return (float)(((std::chrono::steady_clock::now() - lastFrameTime).count()) * nsToSecs);
	}

	inline float FPS()
	{
		return (float)round(1.0f / deltaTime());
	}

	inline float time()
	{
		std::chrono::high_resolution_clock::time_point t = std::chrono::steady_clock::now();
		return (float)(t.time_since_epoch().count() * nsToSecs);
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
	typedef uByte MARKER_INDEX_TYPE;

	// Maximum supported number of memory-markers
	constexpr uByte MAX_MARKER_COUNT = 120;

	// Maximum supported footprint for Athru-controlled CPU-side memory
	// Allocation assumes Athru will use 255 megabytes at most
	const u8Byte STARTING_HEAP_ALLOC = 255000000;

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
	constexpr uByte addrLength()
	{
		uByte addressLen = 32; // Assume 32-bit by default
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
	constexpr uByte halfAddrLength()
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
							 u8Byte,
							 u4Byte>::type addrValType;

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
	static arrayType* ArrayAlloc(u8Byte length,
								 bool setMarker)
	{
		return (arrayType*)AthruCore::Utility::AccessMemory()->AlignedAlloc(length * sizeof(arrayType),
																							  std::alignment_of<arrayType>(),
																							  setMarker);
	}
}

namespace GraphicsStuff
{
	// Display properties
	extern constexpr bool FULL_SCREEN = false;
	extern constexpr bool VSYNC_ENABLED = false;
	extern constexpr u4Byte DISPLAY_WIDTH = 1920;
	extern constexpr u4Byte DISPLAY_HEIGHT = 1080;
	extern constexpr u4Byte DISPLAY_AREA = DISPLAY_WIDTH * DISPLAY_HEIGHT;
	extern constexpr float DISPLAY_ASPECT_RATIO = (float)GraphicsStuff::DISPLAY_WIDTH /
												  (float)GraphicsStuff::DISPLAY_HEIGHT;
	// ASCII key ID for the screenshot button (F)
	extern constexpr u4Byte SCREENSHOT_KEY = 0x46;
}

namespace SceneStuff
{
	extern constexpr u4Byte SYSTEM_COUNT = 100;
	extern constexpr u4Byte BODIES_PER_SYSTEM = 10;
	extern constexpr u4Byte PLANTS_PER_PLANET = 100;
	extern constexpr u4Byte ALIGNED_PARAMETRIC_FIGURES_PER_SYSTEM = 1024; // Total number of parameteric figures/system (number of plants * number
																		  // of bodies, animal forms are defined explicitly with volumes in Athru)
																		  // Aligned to the nearest power of two for simpler buffer management
																		  // (+ at 64 bytes/figure 1024 figures should match exactly to the 65536-byte
																		  // resource alignment requirement in d3d12)
	extern constexpr u4Byte ANIMALS_PER_PLANET = 100;
}
