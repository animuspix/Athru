#pragma once

#include "AppGlobals.h"
#include <functional>
#include <tuple>

namespace AthruGPU
{
	// Supported resource types in Athru
	struct Buffer {};
	struct Texture {};
	struct CBuffer : public Buffer {}; // Only compatible with the [Buffer] resource type
	struct AppBuffer : public Buffer {}; // Only compatible with the [Buffer] resource type
	struct CtrBuffer : public Buffer {}; // Only compatible with the [Buffer] resource type
	template<typename ResrcType> // [ResrcType] must be either [Buffer] or [Texture], enforceable with C++20 concepts
	struct RWResrc : public ResrcType {}; // A read/write allowed resource (GPU-only)
	template<typename ResrcType>
	struct RResrc : public ResrcType {}; // A read-only resource (GPU-only)
	struct UploBuffer : public Buffer {}; // An uploadable resource (CPU-write-once, GPU-read); Athru only allows buffer upload atm
										 // (very few image assets, data assets might be easier to upload in buffers)
	template<typename ReadabilityType> // [ReadabilityType] is either [RResrc] or [RWResrc]
	struct StrmResrc : ReadabilityType {}; // Tiled streaming buffer, mainly allocated within virtual memory and partly loaded into GPU dedicated memory on-demand

	// Supported resource copy states in Athru
	enum class RESRC_COPY_STATES
	{
		SRC,
		NUL,
		DST
	};

	// Considering .As() method for clean resource transitions

	// Typed Athru-specific buffer class
	// Considering predicated dispatch for optimized ray masking: https://docs.microsoft.com/en-us/windows/desktop/direct3d12/predication
	template <typename ContentType,
			  typename AthruResrcType, // Restrict to defined Athru resource types when possible... (see Concepts in C++20)
			  RESRC_COPY_STATES initCopyState, // Resources may be copy-sources, copy-destinations, or neither, but not sources & destinations simultaneously
			  bool crossCmdQueues, // Specify a resource may be shared across command queues
			  bool crossGPUs,
			  ContentType* initDataPttr> // Specify a resource may be shared across command queues
	struct AthruResrc
	{
		public:
			AthruResrc() {}; // Empty default constructor, allows uninitialized resources
			typedef typename std::conditional<std::is_base_of<AthruGPU::Buffer, AthruResrcType>::value,
											  decltype(InitBuf),
											  decltype(InitTex)>::value initFnType;
			initFnType InitResrc = SelFn(); // Conditionally defined initializer, not sure if constructors can be compile-time reduced
										    // as much as procedurally-typed functions
			Microsoft::WRL::ComPtr<ID3D12Resource> buf; // Internal DX12 resource

		private:
			initFnType SelFn =
			[]()
			{
				if constexpr (std::is_base_of<AthruGPU::Buffer, AthruResrcType>::value)
				{ return InitBuf; }
				else if constexpr (std::is_base_of<AthruGPU::Texture, AthruResrcType>::value)
				{ return InitTex; }
			};
			void InitBuf(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
						 const GPUMemory& gpuMem,
						 u4Byte width = 1,
						 DXGI_FORMAT viewFmt = DXGI_FORMAT_UNKNOWN)
			{
				// Allocate buffer memory
				if constexpr (initDataPttr == nullptr && !std::is_same<AthruResrcType, AthruGPU::CBuffer>::value)
				{
					HRESULT hr = gpuMem.AllocBuf(device,
												 width * sizeof(ContentType),
												 resrcDesc<D3D12_RESOURCE_DIMENSION_BUFFER>(),
												 initState(),
												 buf.Get(),
												 AthruGPU::HEAP_TYPES::UPLO); // Constant buffers go to the upload heap for optimized CPU writes
					assert(SUCCEEDED(hr));
				}
				else if constexpr (std::is_same<AthruResrcType, AthruGPU::CBuffer>::value)
				{
					HRESULT hr = gpuMem.AllocBuf(device,
												 width * sizeof(ContentType),
												 resrcDesc<D3D12_RESOURCE_DIMENSION_BUFFER>(),
												 initState(),
												 buf.Get());
					assert(SUCCEEDED(hr));
				}
				else if constexpr (initDataPttr != nullptr)
				{
					// Allocate buffer + upload initial data to the main adapter
					HRESULT hr = gpuMem.ArrayToGPUBuffer<ContentType>(initDataPttr,
																	  width * sizeof(ContentType),
																	  resrcDesc<D3D12_RESOURCE_DIMENSION_BUFFER>(),
																	  initState(),
																	  device,
																	  buf.Get());
					assert(SUCCEEDED(hr));
				}

				// Allocate buffer descriptor/view
			}
			void InitTex(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
						 const GPUMemory& gpuMem,
						 // Assumed no base data for textures (may be neater to format planetary/animal data with
						 // structured buffers instead of textures, afaik DX resources are zeroed by default)
						 u4Byte width = 1,
						 u4Byte height = 1,
						 u4Byte depth = 1,
						 DXGI_FORMAT viewFmt = DXGI_FORMAT_R32G32B32A32_FLOAT,
						 float* clearRGBA = GraphicsStuff::DEFAULT_TEX_CLEAR_VALUE)
			{
				// Convert passed width/height/depth to a DX12 RESOURCE_DIMENSION object
				constexpr auto resrcDimFn =
				[]()
				{
					if constexpr (width > 1) { return D3D12_RESOURCE_DIMENSION_TEXTURE3D; }
					else if constexpr (height > 1) { return D3D12_RESOURCE_DIMENSION_TEXTURE2D; }
					return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
				}

				// Construct an object to capture the color/format we'll splat over the
				// resource each time we clear its contents
				// Assumes no depth-stencil resources (no/limited rasterization in Athru)
				D3D12_CLEAR_COLOR clearColor;
				clearColor.Format = viewFmt;
				clearColor.Color = clearRGBA;

				// Allocate texture memory
				HRESULT hr = gpuMem.AllocTex(device,
											 width * sizeof(ContentType),
											 resrcDesc<resrcDimFn()>(),
											 initState(),
											 clearColor,
											 buf.Get());
				assert(SUCCEEDED(result));

				// Allocate texture descriptor/view
			}
			//void InitResrcInternal(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
			//					   const GPUMemory& gpuMem,
			//					   const void* baseDataPttr = nullptr,
			//					   u4Byte width = 1,
			//					   u4Byte height = 1,
			//					   u4Byte depth = 1;
			//					   DXGI_FORMAT viewFmt = DXGI_FORMAT_UNKNOWN) // Allow optional view-format definition for GPU-viewable,
			//																  // non-structured buffers; default assumes unknown structured data
			//{
			//	// Create view here
			//	//HRESULT result = device->CreateBuffer(&bufferDesc,
			//	//									  safeBaseData[baseDataPttr != nullptr],
			//	//									  buf.GetAddressOf());
			//	//assert(SUCCEEDED(result));
			//
			//	// Create the buffer descriptor
			//	//if constexpr (!(std::is_same<AthruResrcType, AthruGPU::CBuffer>::value ||
			//	//			    std::is_same<AthruResrcType, AthruGPU::StgBuffer>::value))
			//	//{
			//	//	// Choose between unordered-access-view and shader-resource-view construction as appropriate
			//	//	if constexpr (std::is_base_of<AthruGPU::GPURWBuffer, AthruResrcType>::value)
			//	//	{
			//	//		// Generate UAV descriptions, construct UAV
			//	//		D3D11_BUFFER_UAV viewDescA;
			//	//		viewDescA.FirstElement = 0;
			//	//		viewDescA.Flags = bufUAVFlags();
			//	//		viewDescA.NumElements = bufLength;
			//	//
			//	//		D3D11_UNORDERED_ACCESS_VIEW_DESC viewDescB;
			//	//		viewDescB.Format = viewFmt;
			//	//		viewDescB.Buffer = viewDescA;
			//	//		viewDescB.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			//	//		result = device->CreateUnorderedAccessView(buf.Get(), &viewDescB, resrcView.GetAddressOf());
			//	//		assert(SUCCEEDED(result));
			//	//
			//	//		// Generate read-only resource view for write-limited buffers
			//	//		if constexpr (std::is_same<AthruResrcType, AthruGPU::WLimitedBuffer>::value) { CreateCastingSRV(device, bufLength,
			//	//																										viewFmt, result); }
			//	//	}
			//	//	else
			//	//	{
			//	//		CreateCoreSRV(device, bufLength, viewFmt, result);
			//	//	}
			//	//}
			//}
			D3D12_RESOURCE_STATES initState()
			{
				// Initialize state value, set access fields as appropriate
				D3D12_RESOURCE_STATES state = 0;
				bool writeSet = false;
				if constexpr (std::is_same<AthruResrcType, AthruGPU::CBuffer>::value)
				{ state |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER; }
				else if constexpr (std::is_same<AthruResrcType, AthruGPU::RWResrc<AthruGPU::Texture>>::value ||
								   std::is_same<AthruResrcType, AthruGPU::RWResrc<AthruGPU::Buffer>>::value ||
								   std::is_same<AthruResrcType, AthruGPU::AppBuffer>::value) // Write-limited buffers default to write-allowed/[unordered-access]
				{
					state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
					writeSet = true;
				}
				else if constexpr (std::is_same<AthruResrcType, AthruGPU::RResrc<AthruGPU::Buffer>>::value ||
								   std::is_same<AthruResrcType, AthruGPU::RResrc<AthruGPU::Texture>>::value)
				{ state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE; } // Not expecting to use the raster pipeline atm (i.e. no shader-resources inside pixel-shader invocations)
				else if constexpr (std::is_same<AthruResrcType, AthruGPU::UploBuffer>::value)
				{ state = D3D12_RESOURCE_STATE_GENERIC_READ; }

				// Set source/destination copy fields as appropriate
				if constexpr (copyability == RESRC_COPY_STATES::DST)
				{ state = D3D12_RESOURCE_STATE_COPY_DEST; }
				else if constexpr (copyability == RESRC_COPY_STATES::SRC && !writeSet)
				{ state |= D3D12_RESOURCE_STATE_COPY_SOURCE; }
				else if constexpr (copyability == RESRC_COPY_STATES::SRC && writeSet)
				{ state = D3D12_RESOURCE_STATE_COPY_SOURCE; }
				return state; // Return composed state
			}
			D3D12_RESOURCE_FLAGS resrcFlags() // Small function returning appropriate resource flags for different buffer types
			{
				D3D12_RESOURCE_FLAGS flags = 0; // No flags by default
				constexpr bool writable = std::is_same<AthruResrcType, AthruGPU::RWResrc<Buffer>>::value ||
										  std::is_same<AthruResrcType, AthruGPU::RWResrc<Texture>>::value;
				if constexpr (writable) { flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; }
				if constexpr (crossCmdQueues) { flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS; }
				if constexpr (crossGPUs) { flags |= D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER; }
				return flags;
			}
			template<D3D12_RESOURCE_DIMENSION resrcDim, DXGI_FORMAT viewFmt>
			D3D12_RESOURCE_DESC resrcDesc()
			{
				// Construct layout information; prefer driver layout definition for generic buffers, undefined swizzling
				// for tiled/streaming resources and standard DX12 swizzling for generic textures
				D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
				if constexpr (std::is_same<AthruResrcType, AthruGPU::StrmResrc<RWResrc<Texture>>>::value ||
							  std::is_same<AthruResrcType, AthruGPU::StrmResrc<RWResrc<Buffer>>>::value ||
							  std::is_same<AthruResrcType, AthruGPU::StrmResrc<RResrc<Texture>>>::value ||
							  std::is_same<AthruResrcType, AthruGPU::StrmResrc<RResrc<Buffer>>>::value)
				{ layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE; }
				else if constexpr (std::is_base_of<Texture, AthruResrcType>::value)
				{ layout = D3D12_TEXTURE_LAYOUT_64KB_STANDARD_SWIZZLE; }

				// Generate + return the resource description
				D3D12_RESOURCE_DESC desc;
				desc.Dimension = resrcDim;
				desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
				desc.Width = X;
				desc.Height = Y;
				desc.DepthOrArraySize = Z; // Implicitly no texture arrays in Athru
				desc.MipLevels = 0; // All Athru textures are constantly full-res, without mip-mapping
				desc.Format = viewFmt;
				desc.SampleDesc.Count = 1; // We're DIY rendering with minimal rasterization, so no multisampling anywhere
				desc.SampleDesc.Quality = 0;
				desc.Layout = layout;
				desc.Flags = resrcFlags();
				return desc;
			}
			u4Byte bufByteStride() // Small function returning structure byte stride (when appropriate)
			{
				return sizeof(ContentType) * ((std::is_class<ContentType>::value || std::is_same<AthruResrcType, AthruGPU::AppBuffer>::value) &&
											  !(std::is_same<AthruResrcType, AthruGPU::CBuffer>::value)); // Structure-byte-stride is zeroed
																										  // for unstructured and constant
																										  // buffers
			}
	};
}