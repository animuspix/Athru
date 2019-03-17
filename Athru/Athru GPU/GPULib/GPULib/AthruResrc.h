#pragma once

#include "AppGlobals.h"
#include <functional>
#include <tuple>
#include "GPUMemory.h"

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
	template<typename ReadabilityType> // [ReadabilityType] is either [RResrc] or [RWResrc]; uploadable resources should only take the [AthruGPU::Buffer] meta-type
	struct UploBuffer : public ReadabilityType {}; // An uploadable resource (CPU-write-once, GPU-read); Athru only allows buffer upload atm
												   // (very few image assets, data assets might be easier to upload in buffers)
	template<typename ReadabilityType> // [ReadabilityType] is either [RResrc] or [RWResrc]
	struct StrmResrc : ReadabilityType {}; // Tiled streaming buffer, mainly allocated within virtual memory and partly loaded into GPU dedicated memory on-demand

	// Athru resource usage contexts; include rendering, physics, and ecology
	// Used to offset resources into appropriate descriptor tables (if possible, unsure how automated descriptor management is)
	enum class RESRC_CTX
	{
		RNDR_OR_GENERIC,
		PHYSICS_OR_ECO,
		ECOLOGY
	};

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
	class GPUMemory;
	template <typename ContentType,
			  typename AthruResrcType, // Restrict to defined Athru resource types when possible... (see Concepts in C++20)
			  RESRC_COPY_STATES initCopyState, // Resources may be copy-sources, copy-destinations, or neither, but not sources & destinations simultaneously
			  RESRC_CTX resrcContext, // Describe whether a resource is mainly used generically/for rendering, for physics/ecology, or just for ecology
									  // (kinda unintuitive, but simplifies descriptor table organization)
			  ContentType* initDataPttr = nullptr, // Specify initial resource data; only accessed for buffers atm
			  bool crossCmdQueues = false, // Specify a resource may be shared across command queues
			  bool crossGPUs = false> // Specify a resource may be shared across system GPUs
	struct AthruResrc
	{
		public:
			AthruResrc() {}
			Microsoft::WRL::ComPtr<ID3D12Resource> resrc; // Internal DX12 resource
			D3D12_CPU_DESCRIPTOR_HANDLE resrcAddr; // CPU-accessible handle for the descriptor matched with [resrc]
												   // Unsure whether it's reasonable to track this, have asked stack overflow
			// Explicit initializer choices because a single composed initializer becomes invisible to the IDE
			void InitBuf(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
						 const GPUMemory& gpuMem,
						 u4Byte width = 1,
						 DXGI_FORMAT viewFmt = DXGI_FORMAT_UNKNOWN)
			{
				static_assert(!std::is_same<AthruResrcType, AthruGPU::AppBuffer>::value &&
							  !std::is_base_of<AthruGPU::Texture, AthruResrcType>::value,
							  "Generic buffer initialization is only valid for read/write buffers, counter buffers, constant buffers, and uploadable buffers; \
							   textures should be initialized with [InitRWTex(...)] or [InitRTex(...)], and append-consume buffers should be initialized with \
							   [InitAppBuf(...)]");
				InitAllBufs(device,
						    gpuMem,
							width,
							viewFmt);
			}
			void InitAppBuf(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
							const GPUMemory& gpuMem,
							const Microsoft::WRL::ComPtr<ID3D12Resource>* ctrResrc,
							const u4Byte& width,
							const u4Byte& ctrEltOffs = 0,
							DXGI_FORMAT viewFmt = DXGI_FORMAT_UNKNOWN)
			{
				static_assert(std::is_same<AthruResrcType, AthruGPU::AppBuffer>::value,
							  "Append/consume buffer initialization is only valid for stack-like dynamically sized buffers fed from the GPU; generic buffers \
							   should be initialized from [InitBuf(...)], and textures should be initialized with [InitRWTex(...)] or [InitRTex(...)]");
				InitAllBufs(device,
							gpuMem,
							width,
							viewFmt,
							ctrResrc,
							ctrEltOffs);
			}
			void InitRWTex(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
						   const GPUMemory& gpuMem,
						   // Assumed no base data for textures (may be neater to format planetary/animal data with
						   // structured buffers instead of textures, afaik DX resources are zeroed by default)
						   u4Byte width = 1,
						   u4Byte height = 1,
						   u4Byte depth = 1,
						   DXGI_FORMAT viewFmt = DXGI_FORMAT_R32G32B32A32_FLOAT,
						   float* clearRGBA = GraphicsStuff::DEFAULT_TEX_CLEAR_VALUE)
			{
				static_assert(std::is_same<AthruResrcType, AthruGPU::RWResrc<AthruGPU::Texture>>::value,
							  "Read/writable texture initialization is only valid for read/writable resources inheriting from the [AthruGPU::Texture] meta-type; prefer \
							   one of the buffer initializers for buffer resources or [InitRTex(...)] to initialize read-only texture resources");
				InitTex(device,
						gpuMem,
						width,
						height,
						depth,
						viewFmt,
						clearRGBA);
			}
			void InitRTex(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
						  const GPUMemory& gpuMem,
						  // Assumed no base data for textures (may be neater to format planetary/animal data with
						  // structured buffers instead of textures, afaik DX resources are zeroed by default)
						  u4Byte width = 1,
						  u4Byte height = 1,
						  u4Byte depth = 1,
						  DXGI_FORMAT viewFmt = DXGI_FORMAT_R32G32B32A32_FLOAT,
						  D3D12_SHADER_COMPONENT_MAPPING shaderChannelMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
						  float* clearRGBA = GraphicsStuff::DEFAULT_TEX_CLEAR_VALUE)
			{
				static_assert(std::is_same<AthruResrcType, AthruGPU::RResrc<AthruGPU::Texture>>::value,
							  "Read-only texture initialization is only valid for read-only resources inheriting from the [AthruGPU::Texture] meta-type; prefer \
							   one of the buffer initializers for buffer resources or [InitRWTex(...)] to initialize read/writable texture resources");
				InitTex(device,
						gpuMem,
						width,
						height,
						depth,
						viewFmt,
						clearRGBA,
						shaderChannelMapping);
			}
		private:
			void InitAllBufs(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
							 const GPUMemory& gpuMem,
							 u4Byte width = 1,
							 DXGI_FORMAT viewFmt = DXGI_FORMAT_UNKNOWN,
							 const Microsoft::WRL::ComPtr<ID3D12Resource>* ctrResrc = nullptr,
							 const u4Byte& ctrEltOffs = 0)
			{
				// Allocate buffer memory
				if constexpr (initDataPttr == nullptr && !std::is_same<AthruResrcType, AthruGPU::CBuffer>::value)
				{
					HRESULT hr = gpuMem.AllocBuf(device,
												 width * sizeof(ContentType),
												 resrcDesc<D3D12_RESOURCE_DIMENSION_BUFFER, viewFmt>(),
												 initState(),
												 buf.Get(),
												 AthruGPU::HEAP_TYPES::UPLO); // Constant buffers go to the upload heap for optimized CPU writes
					assert(SUCCEEDED(hr));
				}
				else if constexpr (std::is_same<AthruResrcType, AthruGPU::CBuffer>::value)
				{
					HRESULT hr = gpuMem.AllocBuf(device,
												 width * sizeof(ContentType),
												 resrcDesc<D3D12_RESOURCE_DIMENSION_BUFFER, viewFmt>(),
												 initState(),
												 buf.Get());
					assert(SUCCEEDED(hr));
				}
				else if constexpr (initDataPttr != nullptr)
				{
					// Allocate buffer + upload initial data to the main adapter
					HRESULT hr = gpuMem.ArrayToGPUBuffer<ContentType>(initDataPttr,
																	  width * sizeof(ContentType),
																	  resrcDesc<D3D12_RESOURCE_DIMENSION_BUFFER, viewFmt>(),
																	  initState(),
																	  device,
																	  buf.Get());
					assert(SUCCEEDED(hr));
				}

				// Allocate buffer descriptor/view
				// Just provide view-descriptions here, tracking for descriptor offsets + view creation should happen within [GPUMemory]
				// Should provide one descriptor table for rendering, one for physics, and one for ecological simulation
				// (handled by the classes associated with each system, e.g. the rendering descriptor table would be managed by [Renderer])
				// Super ultra fast particle physics thing from last year's SIGGRAPH is open-source now btw!
				// https://github.com/yuanming-hu/taichi_mpm <- just example code, not runnable on Windows/OSX; still great to have for
				// reference (should read the paper too, I have it in my downloads folder as [fastVersatilePhysics])
				// Publication link bc why not: https://www.yzhu.io/projects/siggraph18_mlsmpm_cpic/index.html
				// [AthruResrc] instances should maintain a copy of their handle for validation + external reference
				if constexpr (std::is_same<AthruResrcType, AthruGPU::CBuffer>::value)
				{
					D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc;
					viewDesc.BufferLocation = buf.GetGPUVirtualAddresS();
					viewDesc.SizeInBytes = sizeof(ContentType); // Implicitly require 256-byte alignment for input types
																// possible to scale allocations appropriately, but neater just to maintain some number of padding bytes
																// and consume those whenever I add more members
					resrcAddr = gpuMem->AllocCBV(device, viewDesc);
				}
				else if constexpr (std::is_same<AthruResrcType, AthruGPU::AppBuffer>::value)
				{
					// Assumes a counter resource is always defined for append/consume buffers
					D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc;
					viewDesc.Format = viewFmt;
					viewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
					viewDesc.Buffer.FirstElement = 0;
					viewDesc.Buffer.NumElements = width;
					viewDesc.Buffer.StructureByteStride = sizeof(ContentType);
					viewDesc.Buffer.CounterOffsetInBytes = ctrEltOffs;
					viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE; // No untyped resource views in Athru atm
					resrcAddr = gpuMem->AllocUAV(device,
												 &viewDesc,
												 resrc,
												 ctrResrc); // Append/consume buffers are allocated as standard UAVs with a pointer to a counter resource
				}
				else if constexpr (std::is_same<AthruResrcType, AthruGPU::CtrBuffer>::value ||
								   std::is_base_of<AthruGPU::RWResrc, AthruResrcType>::value)
				{
					D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc;
					viewDesc.Format = viewFmt;
					viewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
					viewDesc.Buffer.FirstElement = 0;
					viewDesc.Buffer.NumElements = width;
					viewDesc.Buffer.StructureByteStride = sizeof(ContentType);
					viewDesc.Buffer.CounterOffsetInBytes = 0;
					viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE; // No untyped resource views in Athru atm
					resrcAddr = gpuMem->AllocUAV(device,
												 &viewDesc,
												 resrc,
												 nullptr);
				}
				else if constexpr (std::is_base_of<AthruGPU::RResrc, AthruResrcType>::value)
				{
					D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc;
					viewDesc.Format = viewFmt;
					viewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
					viewDesc.Buffer.FirstElement = 0;
					viewDesc.Buffer.NumElements = width;
					viewDesc.Buffer.StructureByteStride = 0;
					viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE; // No untyped resource views in Athru atm
					resrcAddr = gpuMem->AllocSRV(device,
												 &viewDesc,
												 resrc,
												 nullptr);
				}
				// -- Descriptor table bindings are handled by game systems (rendering/physics/ecology); return here --
			}
			void InitTex(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
						 const GPUMemory& gpuMem,
						 // Assumed no base data for textures (may be neater to format planetary/animal data with
						 // structured buffers instead of textures, afaik DX resources are zeroed by default)
						 u4Byte width = 1,
						 u4Byte height = 1,
						 u4Byte depth = 1,
						 DXGI_FORMAT viewFmt = DXGI_FORMAT_R32G32B32A32_FLOAT,
						 float* clearRGBA = GraphicsStuff::DEFAULT_TEX_CLEAR_VALUE,
						 D3D12_SHADER_COMPONENT_MAPPING shaderChannelMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING)
			{
				// Construct an object to capture the color/format we'll splat over the
				// resource each time we clear its contents
				// Assumes no depth-stencil resources (no/limited rasterization in Athru)
				D3D12_CLEAR_COLOR clearColor;
				clearColor.Format = viewFmt;
				clearColor.Color = clearRGBA;

				// Allocate texture memory
				HRESULT hr = gpuMem.AllocTex(device,
											 width * sizeof(ContentType),
											 resrcDesc<width, height, depth, viewFmt>(),
											 initState(),
											 clearColor,
											 buf.Get());
				assert(SUCCEEDED(result));

				// Allocate texture descriptor/view
				// Should provide one descriptor table for rendering, one for physics, and one for ecological simulation
				// (handled by the classes associated with each system, e.g. the rendering descriptor table would be managed by [Renderer])
				constexpr bool rwResrc = std::is_base_of<AthruGPU::RWResrc, AthruResrcType>::value;
				typedef typename std::conditional<rwResrc,
												  D3D12_UNORDERED_ACCESS_VIEW_DESC,
												  D3D12_SHADER_RESOURCE_VIEW_DESC>::type viewDescType;
				viewDescType viewDesc = {}; // Prepare view description
				viewDesc.Format = viewFmt;
				if constexpr (!rwResrc) { viewDesc.Shader4ComponentMapping = shaderChannelMapping; } // Shader mappings are settable for shader-resource-views,
																									 // but not unordered-access-views
				if constexpr (width > 1 && height == 1 && depth == 1)
				{
					viewDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1D;
					viewDesc.Texture1D.MostDetailedMip = 0; // DIY rendering in Athru, no real support for mip-mapping
					viewDesc.Texture1D.MipLevels = 1;
					viewDesc.Texture1D.PlaneSlice = 0;
					viewDesc.Texture1D.ResourceMinLODClamp = 0.0f;
				}
				else if constexpr (width > 1 && height > 1 && depth == 1)
				{
					viewDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
					viewDesc.Texture2D.MostDetailedMip = 0; // DIY rendering in Athru, no real support for mip-mapping
					viewDesc.Texture2D.MipLevels = 1;
					viewDesc.Texture2D.PlaneSlice = 0;
					viewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
				}
				else if constexpr (width > 1 && height > 1 && depth > 1)
				{
					viewDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE3D;
					viewDesc.Texture3D.MostDetailedMip = 0; // DIY rendering in Athru, no real support for mip-mapping
					viewDesc.Texture3D.MipLevels = 1;
					viewDesc.Texture3D.PlaneSlice = 0;
					viewDesc.Texture3D.ResourceMinLODClamp = 0.0f;
				}
				if constexpr (rwResrc)
				{
					gpuMem->AllocUAV(device,
									 viewDesc,
									 resrc,
									 nullptr); // Allocate read/writable textures
				}
				else if constexpr (!rwResrc)
				{
					gpuMem->AllocSRV(device,
									 viewDesc,
									 resrc); // Allocate read-only textures
				}
				// -- Descriptor table bindings are handled by game systems (rendering/physics/ecology); return here --
			}
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
			template<int X, int Y, int Z, DXGI_FORMAT viewFmt>
			D3D12_RESOURCE_DESC texResrcDesc()
			{
				// Construct layout information; prefer driver layout definition for generic buffers, undefined swizzling
				// for tiled/streaming resources and standard DX12 swizzling for generic textures
				D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
				if constexpr (std::is_same<AthruResrcType, AthruGPU::StrmResrc<RWResrc<Texture>>>::value ||
							  std::is_same<AthruResrcType, AthruGPU::StrmResrc<RResrc<Texture>>>::value)
				{ layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE; }
				else if constexpr (std::is_base_of<Texture, AthruResrcType>::value)
				{ layout = D3D12_TEXTURE_LAYOUT_64KB_STANDARD_SWIZZLE; }

				// Convert passed width/height/depth to a DX12 RESOURCE_DIMENSION object
				D3D12_RESOURCE_DIMENSION resrcDim = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
				if constexpr (X > 1 && Y > 1 && Z == 1) { resrcDim = D3D12_RESOURCE_DIMENSION_TEXTURE2D; }
				else if constexpr (X > 1 Y > 1 && Z > 1) { resrcDim = D3D12_RESOURCE_DIMENSION_TEXTURE3D; }

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
			template<int width, DXGI_FORMAT viewFmt>
			D3D12_RESOURCE_DESC bufResrcDesc()
			{
				// Construct layout information; prefer driver layout definition for generic buffers, undefined swizzling
				// for tiled/streaming resources and standard DX12 swizzling for generic textures
				D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
				if constexpr (std::is_same<AthruResrcType, AthruGPU::StrmResrc<RWResrc<Buffer>>>::value ||
							  std::is_same<AthruResrcType, AthruGPU::StrmResrc<RResrc<Buffer>>>::value)
				{ layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE; }
				else if constexpr (std::is_base_of<Texture, AthruResrcType>::value)
				{ layout = D3D12_TEXTURE_LAYOUT_64KB_STANDARD_SWIZZLE; }

				// Generate + return the resource description
				D3D12_RESOURCE_DESC desc;
				desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
				desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
				desc.Width = width;
				desc.Height = 1;
				desc.DepthOrArraySize = 1; // Implicitly no texture arrays in Athru
				desc.MipLevels = 0; // All Athru textures are constantly full-res, without mip-mapping
				desc.Format = viewFmt;
				desc.SampleDesc.Count = 1; // We're DIY rendering with minimal rasterization, so no multisampling anywhere
				desc.SampleDesc.Quality = 0;
				desc.Layout = layout;
				desc.Flags = resrcFlags();
				return desc;
			}
	};
}