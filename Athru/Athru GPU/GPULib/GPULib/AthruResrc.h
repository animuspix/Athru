#pragma once

#include "AppGlobals.h"
#include "GPUMemory.h"

namespace AthruGPU
{
	// Supported resource types in Athru
	struct Buffer {};
	struct Texture {};
	struct MessageBuffer : public Buffer {}; // Only compatible with the [Buffer] resource type
											 // The message-buffer stores some constants updated per-frame, and stages system metadata (parameters for planets +
											 // plant species) before transfer to GPU-only memory
    struct ReadbkBuffer : public Buffer {}; // A readback resource (CPU-read, GPU-write); only buffer readback is supported, and readback resources cannot
											// be bound for GPU input (=> they only support the D3D12_RESOURCE_STATES_COPY_DEST usage state)
	struct IndirArgBuffer : public Buffer {}; // A resource used as an argument buffer for indirect draw/dispatch; Athru requires that indirect arguments are never bound
											  // to pipeline state, with copies to/from a generic read/write buffer preferred for updates between indirect command
											  // submissions
	struct AppBuffer : public Buffer {}; // Only compatible with the [Buffer] resource type
	template<typename ResrcType> // [ResrcType] must be either [Buffer] or [Texture], enforceable with C++20 concepts
	struct RWResrc : public ResrcType {}; // A read/write allowed resource (GPU-only)
	template<typename ResrcType>
	struct RResrc : public ResrcType {}; // A read-only resource (GPU-only)
	template<typename ReadabilityType> // [ReadabilityType] is either [RResrc] or [RWResrc]; uploadable resources should only take the [AthruGPU::Buffer] meta-type
	struct UploResrc : public ReadabilityType {}; // An uploadable resource (CPU-write, GPU-read); Athru only allows buffer upload atm
												  // (very few image assets, data assets might be easier to upload in buffers)
	template<typename ReadabilityType> // [ReadabilityType] is either [RResrc] or [RWResrc]
	struct StrmResrc : ReadabilityType {}; // Tiled streaming buffer, mainly allocated within virtual memory and partly loaded into GPU dedicated memory on-demand

	// Convenience function to update expected usage for a given resource; blocks shader execution until
	// change in resource state has completed
	static D3D12_RESOURCE_BARRIER TransitionBarrier(const D3D12_RESOURCE_STATES shiftTo,
											        const Microsoft::WRL::ComPtr<ID3D12Resource>& resrc,
											        const D3D12_RESOURCE_STATES shiftFrom)
	{
		D3D12_RESOURCE_BARRIER barrier;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = resrc.Get();
		barrier.Transition.StateBefore = shiftFrom;
		barrier.Transition.StateAfter = shiftTo;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE; // Use [begin] and [end] flags for split barriers
		barrier.Transition.Subresource = 0; // No mipmaps in Athru
		return barrier;
	}

	// Similar to above, but blocks uav accesses betweeen dispatches/draw-calls; note that shader executions
	// overlap by default in D3D12 (needed AMD/NVIDIA extensions in D3D11)
	static D3D12_RESOURCE_BARRIER UAVBarrier(const Microsoft::WRL::ComPtr<ID3D12Resource>& resrc)
	{
		D3D12_RESOURCE_BARRIER barrier;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.UAV.pResource = resrc.Get();
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		return barrier;
	}

	// Typed Athru-specific buffer class
	// Considering predicated dispatch for optimized ray masking: https://docs.microsoft.com/en-us/windows/desktop/direct3d12/predication
	template <typename ContentType,
			  typename AthruResrcType, // Restrict to defined Athru resource types when possible... (see Concepts in C++20)
			  ContentType* initDataPttr = nullptr, // Specify initial resource data; only accessed for buffers atm
			  bool crossCmdQueues = false, // Specify a resource may be shared across command queues
			  bool crossGPUs = false> // Specify a resource may be shared across system GPUs
	struct AthruResrc
	{
		public:
			AthruResrc()
			{
				resrcState = D3D12_RESOURCE_STATE_COMMON;
				if constexpr (std::is_same<AthruResrcType, AthruGPU::RWResrc<AthruGPU::Texture>>::value ||
					std::is_same<AthruResrcType, AthruGPU::RWResrc<AthruGPU::Buffer>>::value ||
					std::is_same<AthruResrcType, AthruGPU::AppBuffer>::value) // Write-limited buffers default to write-allowed/[unordered-access]
				{
					resrcState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
				}
				else if constexpr (std::is_same<AthruResrcType, AthruGPU::RResrc<AthruGPU::Buffer>>::value ||
								   std::is_same<AthruResrcType, AthruGPU::RResrc<AthruGPU::Texture>>::value)
				{
					resrcState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
				} // Not expecting to use the raster pipeline atm (i.e. no shader-resources inside pixel-shader invocations)
				else if constexpr (std::is_same<AthruResrcType, AthruGPU::UploResrc<AthruGPU::RResrc<AthruGPU::Buffer>>>::value ||
								   std::is_same<AthruResrcType, AthruGPU::UploResrc<AthruGPU::RWResrc<AthruGPU::Buffer>>>::value ||
								   std::is_same<AthruResrcType, AthruGPU::UploResrc<AthruGPU::RResrc<AthruGPU::Texture>>>::value ||
								   std::is_same<AthruResrcType, AthruGPU::UploResrc<AthruGPU::RWResrc<AthruGPU::Texture>>>::value ||
								   std::is_same<AthruResrcType, AthruGPU::MessageBuffer>::value)
				{
					resrcState = D3D12_RESOURCE_STATE_GENERIC_READ;
				}
				else if constexpr (std::is_same<AthruResrcType, AthruGPU::IndirArgBuffer>::value)
				{
					resrcState = D3D12_RESOURCE_STATE_COPY_DEST;
				}
			}
            ~AthruResrc() {}

			Microsoft::WRL::ComPtr<ID3D12Resource> resrc = nullptr; // Internal DX12 resource
			D3D12_CPU_DESCRIPTOR_HANDLE resrcViewAddr = { }; // CPU-accessible handle for the descriptor matched with [resrc]
															 // Resource views associated with the message buffer are strictly bound over constant inputs
															 // (the first ~256 bytes) because the remaining message space is empty per-frame and used to
															 // stage scene metadata before transferring it to gpu-only memory for higher
															 // performance

			// D3D12 resource state at any particular time (render-target, unordered access, non-pixel shader resource...)
			D3D12_RESOURCE_STATES resrcState = D3D12_RESOURCE_STATE_COMMON;

			// Explicit initializer choices because a single composed initializer becomes invisible to the IDE
			//////////////////////////////////////////////////////////////////////////////////////////////////

			// Initialize a generic read-only or read/write buffer resource on GPU-preferred memory
			void InitBuf(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
						 GPUMemory& gpuMem,
						 u4Byte width = GraphicsStuff::TILING_AREA,
						 DXGI_FORMAT viewFmt = DXGI_FORMAT_UNKNOWN)
			{
				InitAllBufs(device,
						    gpuMem,
							width,
							viewFmt);
			}

			// Initialize the CPU->GPU message buffer
            void InitMsgBuf(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
						    GPUMemory& gpuMem,
                            address* mappedCPUAddr) // The CPU-side address that will process message writes
			{
                // Constant buffers use the same generalized initializer as generic buffers, but with unspecified hardware format and exactly one element
                // (since Athru only expects to use constant buffers as structured lists of character/numeric shader inputs, and not for e.g. passing
                // CPU-driven point clouds to the GPU)
				InitAllBufs(device,
						    gpuMem,
							65536, // Buffers are 65536-byte aligned and this is likely to be the zeroth one allocated in the upload heap, so specify that many
								   // elements here (the message buffer is a generic bag-of-bytes with no specific format)
							DXGI_FORMAT_UNKNOWN);

                // Immediately map the message buffer after initialization
                D3D12_RANGE range;
                range.Begin = 0;
                range.End = 0; // The message buffer is implicitly write-only for the CPU
                HRESULT hr = resrc->Map(0, &range, mappedCPUAddr);
                assert(SUCCEEDED(hr));
			}

			// Initialize the readback buffer
			void InitReadbkBuf(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
							   GPUMemory& gpuMem)
			{
				// No bindable views, so just allocate the readback buffer and test for errors before returning
				/////////////////////////////////////////////////////////////////////////////////////////////////////

				// Allocate the buffer + record resource state
				HRESULT hr = gpuMem.AllocBuf(device, AthruGPU::EXPECTED_SHARED_GPU_RDBK_MEM,
											bufResrcDesc(AthruGPU::EXPECTED_SHARED_GPU_RDBK_MEM, DXGI_FORMAT_UNKNOWN),
											D3D12_RESOURCE_STATE_COPY_DEST, resrc,
											AthruGPU::HEAP_TYPES::READBACK);
				resrcState = D3D12_RESOURCE_STATE_COPY_DEST;
				assert(SUCCEEDED(hr));

				// Send [resrcViewAddr] to the zeroth address since we'll never bind any views to the readback buffer
				resrcViewAddr.ptr = NULL;
			}

			// Initialize an append/consume buffer resource
			void InitAppBuf(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
							GPUMemory& gpuMem,
							const Microsoft::WRL::ComPtr<ID3D12Resource>& ctrResrc,
							const u4Byte& width = GraphicsStuff::TILING_AREA,
							const u4Byte& ctrEltOffs = 0,
							DXGI_FORMAT viewFmt = DXGI_FORMAT_UNKNOWN)
			{
				InitAllBufs(device,
							gpuMem,
							width,
							viewFmt,
							ctrResrc,
							ctrEltOffs);
			}

			// Initialize a read/write buffer resource in GPU-preferred memory without committing to a view
			void InitRawRWBuf(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
							  GPUMemory& gpuMem,
							  const u4Byte& numBytes)
			{
				HRESULT hr = gpuMem.AllocBuf(device, numBytes,
											 bufResrcDesc(numBytes, DXGI_FORMAT_UNKNOWN), resrcState, resrc,
											 AthruGPU::HEAP_TYPES::GPU_ACCESS_ONLY);
				assert(SUCCEEDED(hr));

				// Send [resrcViewAddr] to the zeroth address since we'll be generating the views associated with each raw buffer separately
				// to the raw buffers themselves
				resrcViewAddr.ptr = NULL;
			}

			// Initialize an append/consume view over a given buffer resource
			void InitAppProj(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
							 GPUMemory& gpuMem,
							 const Microsoft::WRL::ComPtr<ID3D12Resource>& targetResrc,
							 const Microsoft::WRL::ComPtr<ID3D12Resource>& ctrResrc,
							 const u4Byte& width = GraphicsStuff::TILING_AREA,
							 const u4Byte& ctrEltOffs = 0,
							 const u4Byte& viewOffs = 0,
							 DXGI_FORMAT viewFmt = DXGI_FORMAT_UNKNOWN)
			{
				D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc;
				viewDesc.Format = viewFmt;
				viewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
				viewDesc.Buffer.FirstElement = viewOffs / sizeof(ContentType);
				viewDesc.Buffer.NumElements = width;
				viewDesc.Buffer.StructureByteStride = sizeof(ContentType);
				viewDesc.Buffer.CounterOffsetInBytes = ctrEltOffs;
				viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE; // No untyped resource views in Athru atm
				resrcViewAddr = gpuMem.AllocUAV(device,
											    &viewDesc,
											    targetResrc,
											    ctrResrc); // Append/consume buffers are allocated as standard UAVs with a pointer to a counter resource
				resrc = nullptr; // Projections are pure views over resources defined externally, so nullify the local resource here
			}

			// Initialize a standard read/write view over a given buffer resource
			void InitRWBufProj(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
							   GPUMemory& gpuMem,
							   const Microsoft::WRL::ComPtr<ID3D12Resource>& targetResrc,
							   const u4Byte& width = GraphicsStuff::TILING_AREA,
							   const u4Byte& viewOffs = 0,
							   DXGI_FORMAT viewFmt = DXGI_FORMAT_UNKNOWN)
			{
				D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc;
				viewDesc.Format = viewFmt;
				viewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
				viewDesc.Buffer.NumElements = width;
				viewDesc.Buffer.StructureByteStride = 0;
				if constexpr (std::is_class<ContentType>::value &&
							  !std::is_same<ContentType, DirectX::XMFLOAT4>::value &&
							  !std::is_same<ContentType, DirectX::XMVECTOR>::value)
				{ viewDesc.Buffer.StructureByteStride = sizeof(ContentType); }
				viewDesc.Buffer.FirstElement = viewOffs / sizeof(ContentType);
				viewDesc.Buffer.CounterOffsetInBytes = 0;
				viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE; // No untyped resource views in Athru atm
				resrcViewAddr = gpuMem.AllocUAV(device,
											    &viewDesc,
											    targetResrc,
											    nullptr);
				resrc = nullptr; // Projections are pure views over resources defined externally, so nullify the local resource here
			}

			// Initialize a read/writable texture resource
			void InitRWTex(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
						   GPUMemory& gpuMem,
						   // Assumed no base data for textures (may be neater to format planetary/animal data with
						   // structured buffers instead of textures, afaik DX resources are zeroed by default)
						   u4Byte width = 1,
						   u4Byte height = 1,
						   u4Byte depth = 1,
						   DXGI_FORMAT viewFmt = DXGI_FORMAT_R32G32B32A32_FLOAT,
						   const float* clearRGBA = GraphicsStuff::DEFAULT_TEX_CLEAR_VALUE)
			{
				InitTex(device,
						gpuMem,
						width,
						height,
						depth,
						viewFmt,
						clearRGBA);
			}

			// Initialize a read-only texture resource
			void InitRTex(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
						  GPUMemory& gpuMem,
						  // Assumed no base data for textures (may be neater to format planetary/animal data with
						  // structured buffers instead of textures, afaik DX resources are zeroed by default)
						  u4Byte width = 1,
						  u4Byte height = 1,
						  u4Byte depth = 1,
						  DXGI_FORMAT viewFmt = DXGI_FORMAT_R32G32B32A32_FLOAT,
						  const float* clearRGBA = GraphicsStuff::DEFAULT_TEX_CLEAR_VALUE,
                          D3D12_SHADER_COMPONENT_MAPPING shaderChannelMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING)
			{
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
							 GPUMemory& gpuMem,
							 u4Byte width = 1,
							 DXGI_FORMAT viewFmt = DXGI_FORMAT_UNKNOWN,
							 const Microsoft::WRL::ComPtr<ID3D12Resource>& ctrResrc = nullptr,
							 const u4Byte& ctrEltOffs = 0)
			{
				// Allocate buffer memory
				if constexpr (initDataPttr == nullptr)
				{
					HRESULT hr = gpuMem.AllocBuf(device,
												 width * sizeof(ContentType),
												 bufResrcDesc(width * sizeof(ContentType), viewFmt),
												 resrcState,
												 resrc,
				                                 bufHeapType());
					assert(SUCCEEDED(hr));
				}
				else if constexpr (initDataPttr != nullptr)
				{
					// Allocate buffer + upload initial data to the main adapter
					HRESULT hr = gpuMem.ArrayToGPUBuffer<ContentType, bufHeapType()>(initDataPttr,
																	                 width * sizeof(ContentType),
																	                 bufResrcDesc(width, viewFmt1),
																	                 resrcState,
																	                 device,
																	                 resrc.Get());
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
				if constexpr (std::is_same<AthruResrcType, AthruGPU::MessageBuffer>::value)
				{
					D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc;
					viewDesc.BufferLocation = resrc->GetGPUVirtualAddress();
					viewDesc.SizeInBytes = AthruGPU::EXPECTED_GPU_CONSTANT_MEM;
					resrcViewAddr = gpuMem.AllocCBV(device, &viewDesc);
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
					resrcViewAddr = gpuMem.AllocUAV(device,
												    &viewDesc,
												    resrc,
												    ctrResrc); // Append/consume buffers are allocated as standard UAVs with a pointer to a counter resource
				}
				else if constexpr (std::is_base_of<AthruGPU::RWResrc<AthruGPU::Buffer>, AthruResrcType>::value)
				{
					D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc;
					viewDesc.Format = viewFmt;
					viewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
					viewDesc.Buffer.FirstElement = 0;
					viewDesc.Buffer.NumElements = width;
					viewDesc.Buffer.StructureByteStride = 0;
					if constexpr (std::is_class<ContentType>::value)
					{ viewDesc.Buffer.StructureByteStride = sizeof(ContentType); }
					viewDesc.Buffer.CounterOffsetInBytes = 0;
					viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE; // No untyped resource views in Athru atm
					resrcViewAddr = gpuMem.AllocUAV(device,
											        &viewDesc,
											        resrc,
											        nullptr);
				}
				else if constexpr (std::is_base_of<AthruGPU::RResrc<AthruGPU::Buffer>, AthruResrcType>::value)
				{
					D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc;
					viewDesc.Format = viewFmt;
					viewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
					viewDesc.Buffer.FirstElement = 0;
					viewDesc.Buffer.NumElements = width;
					viewDesc.Buffer.StructureByteStride = 0;
					viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					if constexpr (std::is_class<ContentType>::value)
					{ viewDesc.Buffer.StructureByteStride = sizeof(ContentType); }
					viewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE; // No untyped resource views in Athru atm
					resrcViewAddr = gpuMem.AllocSRV(device,
												    &viewDesc,
												    resrc);
				}
			}
			void InitTex(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
						 GPUMemory& gpuMem,
						 u4Byte width,
						 u4Byte height,
						 u4Byte depth,
						 // Assumed no base data for textures (may be neater to format planetary/animal data with
						 // structured buffers instead of textures, afaik DX resources are zeroed by default)
						 DXGI_FORMAT viewFmt = DXGI_FORMAT_R32G32B32A32_FLOAT,
						 const float* clearRGBA = GraphicsStuff::DEFAULT_TEX_CLEAR_VALUE,
						 D3D12_SHADER_COMPONENT_MAPPING shaderChannelMapping = (D3D12_SHADER_COMPONENT_MAPPING)D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING)
			{
				// Allocate texture memory
				HRESULT hr = gpuMem.AllocTex(device,
											 width * sizeof(ContentType),
											 texResrcDesc(width, height, depth, viewFmt),
											 resrcState,
											 resrc);
				assert(SUCCEEDED(hr));

				// Allocate texture descriptor/view
				// Should provide one descriptor table for rendering, one for physics, and one for ecological simulation
				// (handled by the classes associated with each system, e.g. the rendering descriptor table would be managed by [Renderer])
				constexpr bool rwResrc = std::is_base_of<AthruGPU::RWResrc<AthruGPU::Texture>, AthruResrcType>::value ||
                                         std::is_base_of<AthruGPU::RWResrc<AthruGPU::Buffer>, AthruResrcType>::value;
				typedef typename std::conditional<rwResrc,
												  D3D12_UNORDERED_ACCESS_VIEW_DESC,
												  D3D12_SHADER_RESOURCE_VIEW_DESC>::type viewDescType;
				viewDescType viewDesc = {};
				viewDesc.Format = viewFmt;
				if constexpr (!rwResrc) { viewDesc.Shader4ComponentMapping = shaderChannelMapping; } // Shader mappings are settable for shader-resource-views,
																									 // but not unordered-access-views
				if constexpr (rwResrc)
				{
                    // DIY surfaces in Athru, no real support for mip-mapping
                    if (width > 1 && height == 1 && depth == 1)
                    {
						viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
                        viewDesc.Texture1D.MipSlice = 0;
                    }
                    else if (width > 1 && height > 1 && depth == 1)
                    {
						viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                        viewDesc.Texture2D.MipSlice = 0;
                        viewDesc.Texture2D.PlaneSlice = 0;
                    }
                    else if (width > 1 && height > 1 && depth > 1)
                    {
						viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
                        viewDesc.Texture3D.MipSlice = 0;
                        viewDesc.Texture3D.FirstWSlice = 0;
                        viewDesc.Texture3D.WSize = 0;
                    }

                    // Allocate read/writable textures
					gpuMem.AllocUAV(device,
									&viewDesc,
									resrc,
									nullptr);
				}
				else if constexpr (!rwResrc)
				{
                    // Match resource & view dimensions
                    if (width > 1 && height == 1 && depth == 1)
                    { viewDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1D; }
                    else if (width > 1 && height > 1 && depth == 1)
                    { viewDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D; }
                    else if (width > 1 && height > 1 && depth > 1)
                    { viewDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE3D; }

                    // Allocate read-only textures
					gpuMem.AllocSRV(device,
									viewDesc,
									resrc);
				}
			}
			D3D12_RESOURCE_FLAGS resrcFlags() // Small function returning appropriate resource flags for different buffer types
			{
				D3D12_RESOURCE_FLAGS flags = (D3D12_RESOURCE_FLAGS)0; // No flags by default
				constexpr bool writable = std::is_same<AthruResrcType, AthruGPU::RWResrc<Buffer>>::value ||
										  std::is_same<AthruResrcType, AthruGPU::RWResrc<Texture>>::value;
				if constexpr (writable) { flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; }
				if constexpr (crossCmdQueues) { flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS; }
				if constexpr (crossGPUs) { flags |= D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER; }
				return flags;
			}
			D3D12_RESOURCE_DESC texResrcDesc(int X, int Y, int Z, DXGI_FORMAT viewFmt)
			{
				// Construct layout information; prefer undefined swizzling for tiled/streaming resources
                // and standard DX12 swizzling for generic textures
				// Should test for support + optionally provide standard swizzle here in future
				D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
				//if constexpr (std::is_same<AthruResrcType, AthruGPU::StrmResrc<RWResrc<Texture>>>::value ||
				//			  std::is_same<AthruResrcType, AthruGPU::StrmResrc<RResrc<Texture>>>::value)
				//{ layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE; }

				// Convert passed width/height/depth to a DX12 RESOURCE_DIMENSION object
				D3D12_RESOURCE_DIMENSION resrcDim = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
				if (X > 1 && Y > 1 && Z == 1) { resrcDim = D3D12_RESOURCE_DIMENSION_TEXTURE2D; }
				else if (X > 1 && Y > 1 && Z > 1) { resrcDim = D3D12_RESOURCE_DIMENSION_TEXTURE3D; }

				// Generate + return the resource description
				D3D12_RESOURCE_DESC desc;
				desc.Dimension = resrcDim;
				desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
				desc.Width = X;
				desc.Height = Y;
				desc.DepthOrArraySize = Z; // Implicitly no texture arrays in Athru
				desc.MipLevels = 1; // All Athru textures are constantly full-res, without mip-mapping
				desc.Format = viewFmt;
				desc.SampleDesc.Count = 1; // We're DIY rendering with minimal rasterization, so no multisampling anywhere
				desc.SampleDesc.Quality = 0;
				desc.Layout = layout;
				desc.Flags = resrcFlags();
				return desc;
			}
			D3D12_RESOURCE_DESC bufResrcDesc(const u4Byte& width,
                                             const DXGI_FORMAT& viewFmt)
			{
				// Construct layout information; prefer row-major layout definition for generic buffers and undefined swizzling
				// for tiled/streaming resources
				D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
				if constexpr (std::is_same<AthruResrcType, AthruGPU::StrmResrc<RWResrc<Buffer>>>::value ||
							  std::is_same<AthruResrcType, AthruGPU::StrmResrc<RResrc<Buffer>>>::value)
				{ layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE; }

				// Generate + return the resource description
				D3D12_RESOURCE_DESC desc;
				desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
				desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
				desc.Width = width;
				desc.Height = 1;
				desc.DepthOrArraySize = 1; // Implicitly no texture arrays in Athru
				desc.MipLevels = 1; // All Athru textures are constantly full-res, without mip-mapping
				desc.Format = viewFmt;
				desc.SampleDesc.Count = 1; // We're DIY rendering with minimal rasterization, so no multisampling anywhere
				desc.SampleDesc.Quality = 0;
				desc.Layout = layout;
				desc.Flags = resrcFlags();
				return desc;
			}
            AthruGPU::HEAP_TYPES bufHeapType()
            {
                if constexpr ((!std::is_same<AthruResrcType, AthruGPU::UploResrc<AthruGPU::RResrc<AthruGPU::Buffer>>>::value ||
                               !std::is_same<AthruResrcType, AthruGPU::UploResrc<AthruGPU::RWResrc<AthruGPU::Buffer>>>::value) &&
                              !std::is_same<AthruResrcType, AthruGPU::MessageBuffer>::value)
                { return AthruGPU::HEAP_TYPES::GPU_ACCESS_ONLY; }
                else if constexpr ((std::is_same<AthruResrcType, AthruGPU::UploResrc<AthruGPU::RResrc<AthruGPU::Buffer>>>::value ||
                                    std::is_same<AthruResrcType, AthruGPU::UploResrc<AthruGPU::RWResrc<AthruGPU::Buffer>>>::value) ||
                                   std::is_same<AthruResrcType, AthruGPU::MessageBuffer>::value)
                { return AthruGPU::HEAP_TYPES::UPLO; }
                else if constexpr (std::is_same<AthruResrcType, AthruGPU::ReadbkBuffer>::value ||
                                   !std::is_same<AthruResrcType, AthruGPU::MessageBuffer>::value)
                { return AthruGPU::HEAP_TYPES::READBACK; }
            }
	};
}