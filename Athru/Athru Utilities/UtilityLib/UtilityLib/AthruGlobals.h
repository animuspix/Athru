#pragma once

#include <chrono>
#include <math.h>
#include <assert.h>
#include <d3d11.h>
#include "Typedefs.h"
#include <wrl\client.h>

namespace TimeStuff
{
	// Initialise the "previous" time to zero (since the
	// game starts with no previous frame and has to work
	// from there)
	extern std::chrono::steady_clock::time_point timeAtLastFrame;

	// [inline] instead of a macro so we get type-safety :)
	inline float deltaTime()
	{
		std::chrono::steady_clock::time_point current = std::chrono::steady_clock::now();
		std::chrono::duration<float> deltaTimeValue = std::chrono::duration_cast<std::chrono::duration<float>>(current - timeAtLastFrame);
		return deltaTimeValue.count();
	}

	inline float FPS()
	{
		return std::roundf(1.0f / deltaTime());
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
	extern constexpr fourByteUnsigned DISPLAY_WIDTH = 1024;
	extern constexpr fourByteUnsigned DISPLAY_HEIGHT = 768;
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
	extern constexpr fourByteUnsigned PROG_PASSES_PER_IMAGE = 5;
	extern constexpr fourByteUnsigned GROUP_WIDTH_PATH_REDUCTION = 16;
	extern constexpr fourByteUnsigned GROUP_HEIGHT_PATH_REDUCTION = 16;
	extern constexpr fourByteUnsigned GROUP_AREA_PATH_REDUCTION = 256;
	extern constexpr fourByteUnsigned MAX_NUM_BOUNCES = 5;
	extern constexpr fourByteUnsigned NUM_DIRECT_SAMPLES = 8;
	extern constexpr fourByteUnsigned NUM_INDIRECT_SAMPLES = 1;
	extern constexpr fourByteUnsigned NUM_AA_SAMPLES = 256;
}

namespace SceneStuff
{
	extern constexpr fourByteUnsigned SYSTEM_COUNT = 100;
	extern constexpr fourByteUnsigned MAX_NUM_SCENE_FIGURES = 10;
	extern constexpr fourByteUnsigned MAX_NUM_SCENE_ORGANISMS = 8;
}

namespace PlantStuff
{
	extern constexpr fourByteUnsigned MAX_PLANT_AGE = 200;
	extern constexpr float MAX_PLANT_SIZE = 25.0f;
	extern constexpr float MAX_TROPISM = 0.4f;
	extern constexpr fourByteUnsigned MAX_BRANCH_COUNT_AT_FORK = 10;
	extern constexpr fourByteUnsigned MAX_LEAF_LOBE_COUNT = 4;
}

namespace CritterStuff
{
}

namespace GPGPUStuff
{
	extern constexpr fourByteUnsigned NUM_RAND_SEEDS = 88917504;

	// Construct a GPGPU read/write buffer with the given complex
	// (non-pointer, non-enum, non-array, non-reference) type
	template<typename BufType>
	static void BuildRWStructBuffer(const Microsoft::WRL::ComPtr<ID3D11Device>& device,
									Microsoft::WRL::ComPtr<ID3D11Buffer>& bufPttr,
									const D3D11_SUBRESOURCE_DATA* baseDataPttr,
									Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& rwView,
									D3D11_BUFFER_UAV_FLAG uavFlags,
									fourByteUnsigned bufLength)
	{
		// Describe the buffer we're creating and storing at
		// [bufPttr]
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = sizeof(BufType) * bufLength;
		bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		bufferDesc.StructureByteStride = sizeof(BufType);

		// Construct a buffer at [bufPttr] with the description given above
		HRESULT result = device->CreateBuffer(&bufferDesc,
											  baseDataPttr,
											  bufPttr.GetAddressOf());
		assert(SUCCEEDED(result));

		// Describe the the shader-friendly read/write resource view we'll
		// use to access the buffer during general-purpose graphics
		// processing
		D3D11_BUFFER_UAV viewDescA;
		viewDescA.FirstElement = 0;
		viewDescA.Flags = uavFlags;
		viewDescA.NumElements = bufLength;

		D3D11_UNORDERED_ACCESS_VIEW_DESC viewDescB;
		viewDescB.Format = DXGI_FORMAT_UNKNOWN;
		viewDescB.Buffer = viewDescA;
		viewDescB.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;

		// Construct a DirectX11 "view" over the data at [bufPttr]
		result = device->CreateUnorderedAccessView(bufPttr.Get(), &viewDescB, rwView.GetAddressOf());
		assert(SUCCEEDED(result));
	}

	// Construct a GPGPU read-write buffer with the given primitive type
	template<typename BufType>
	static void BuildRWBuffer(const Microsoft::WRL::ComPtr<ID3D11Device>& device,
							  Microsoft::WRL::ComPtr<ID3D11Buffer>& bufPttr,
							  const D3D11_SUBRESOURCE_DATA* baseDataPttr,
							  Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& rwView,
							  DXGI_FORMAT& viewFormat,
					 		  D3D11_BUFFER_UAV_FLAG uavFlags,
							  fourByteUnsigned& bufLength)
	{
		// Describe the buffer we're creating and storing at
		// [bufPttr]
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = sizeof(BufType) * bufLength;
		bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;

		// Construct the state buffer using the seeds + description
		// defined above
		HRESULT result = device->CreateBuffer(&bufferDesc,
											  baseDataPttr,
											  bufPttr.GetAddressOf());
		assert(SUCCEEDED(result));

		// Describe the the shader-friendly read/write resource view we'll
		// use to access the buffer during general-purpose graphics
		// processing
		D3D11_BUFFER_UAV viewDescA;
		viewDescA.FirstElement = 0;
		viewDescA.Flags = uavFlags;
		viewDescA.NumElements = bufLength;

		D3D11_UNORDERED_ACCESS_VIEW_DESC viewDescB;
		viewDescB.Format = viewFormat;
		viewDescB.Buffer = viewDescA;
		viewDescB.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;

		// Construct a DirectX11 "view" over the data at [bufPttr]
		result = device->CreateUnorderedAccessView(bufPttr.Get(), &viewDescB, rwView.GetAddressOf());
		assert(SUCCEEDED(result));
	}

	// Construct a GPGPU staging buffer with the given primitive
	// type
	template<typename BufType>
	static void BuildStagingBuffer(const Microsoft::WRL::ComPtr<ID3D11Device>& device,
								   Microsoft::WRL::ComPtr<ID3D11Buffer>& bufPttr,
								   fourByteUnsigned bufLength)
	{
		// Describe the buffer we're creating and storing at
		// [bufPttr]
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage = D3D11_USAGE_STAGING;
		bufferDesc.ByteWidth = sizeof(BufType) * bufLength;
		bufferDesc.BindFlags = 0;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;

		// Construct a buffer at [bufPttr] with the description given above
		HRESULT result = device->CreateBuffer(&bufferDesc,
											  nullptr,
											  bufPttr.GetAddressOf());
		assert(SUCCEEDED(result));
	}

	// Construct a GPGPU staging buffer with the given complex
	// (non-pointer, non-enum, non-array, non-reference) type
	template<typename BufType>
	static void BuildStagingStructBuffer(const Microsoft::WRL::ComPtr<ID3D11Device>& device,
										 Microsoft::WRL::ComPtr<ID3D11Buffer>& bufPttr,
										 fourByteUnsigned bufLength)
	{
		// Describe the buffer we're creating and storing at
		// [bufPttr]
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage = D3D11_USAGE_STAGING;
		bufferDesc.ByteWidth = sizeof(BufType) * bufLength;
		bufferDesc.BindFlags = 0;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		bufferDesc.StructureByteStride = sizeof(BufType);

		// Construct a buffer at [bufPttr] with the description given above
		HRESULT result = device->CreateBuffer(&bufferDesc,
											  nullptr,
											  bufPttr.GetAddressOf());
		assert(SUCCEEDED(result));
	}
}