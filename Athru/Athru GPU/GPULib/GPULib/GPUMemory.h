#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include "GPUGlobals.h"

namespace AthruGPU
{
	enum class SHADER_CTX
	{
		RNDR,
		PHYS,
		ECOL
	};
}

namespace AthruGPU
{
	class GPUMemory
	{
		public:
			GPUMemory(const Microsoft::WRL::ComPtr<ID3D12Device>& device);
			~GPUMemory();

			// [GPUMemory] expects aligned Buffer/Texture/View allocations
			HRESULT AllocBuf(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
							 const u4Byte& bufSize,
							 const D3D12_RESOURCE_DESC bufDesc,
							 const D3D12_RESOURCE_STATES initState,
							 const Microsoft::WRL::ComPtr<ID3D12Resource>& bufPttr,
							 const AthruGPU::HEAP_TYPES = AthruGPU::HEAP_TYPES::GPU_ACCESS_ONLY);

			// [GPUMemory] expects aligned Buffer/Texture/View allocations
			HRESULT AllocTex(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
							 const u4Byte& bufSize,
							 uByte* initDataPttr,
							 const D3D12_RESOURCE_DESC texDesc,
							 const D3D12_RESOURCE_STATES initState,
							 const Microsoft::WRL::ComPtr<ID3D12Resource>& texPttr);

			// Allocate a shader view over a resource described by [viewDesc], and return a handle to the allocated view
			D3D12_CPU_DESCRIPTOR_HANDLE GPUMemory::AllocCBV(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
															const D3D12_CONSTANT_BUFFER_VIEW_DESC* viewDesc);
			D3D12_CPU_DESCRIPTOR_HANDLE GPUMemory::AllocSRV(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
															const D3D12_SHADER_RESOURCE_VIEW_DESC* viewDesc,
															const Microsoft::WRL::ComPtr<ID3D12Resource>& resrc);
			D3D12_CPU_DESCRIPTOR_HANDLE GPUMemory::AllocUAV(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
															const D3D12_UNORDERED_ACCESS_VIEW_DESC* viewDesc,
															const Microsoft::WRL::ComPtr<ID3D12Resource>& dataResrc,
															const Microsoft::WRL::ComPtr<ID3D12Resource>& ctrResrc);

            // Return a reference to the shader descriptor heap ([shaderViewMem])
            const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& GetShaderViewMem();

			// Return a GPU handle to the descriptors associated with the given resource context
			const D3D12_GPU_DESCRIPTOR_HANDLE GetBaseDescriptor(const SHADER_CTX ctx);

			// Overload the standard allocation/de-allocation operators
			void* operator new(size_t size);
			void operator delete(void* target);
		private:
			// Generalized shader-visible view allocator
			// Implementable because we only expect to allocate shader-resource views, unordered-access views, and
			// constant-buffer-views in Athru
			template<typename viewDescType> // Should constrain to cbv/uav/srv description types with C++20 concepts
			D3D12_CPU_DESCRIPTOR_HANDLE AllocView(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
											      const viewDescType* viewDesc,
											      const Microsoft::WRL::ComPtr<ID3D12Resource>& dataResrc = nullptr,
											      const Microsoft::WRL::ComPtr<ID3D12Resource>& ctrResrc = nullptr)
			{
				// Cache view handle
				D3D12_CPU_DESCRIPTOR_HANDLE viewHandle = shaderViewMem.offs;

				// Allocate view & return descriptor handle
				if constexpr (std::is_same<viewDescType, D3D12_CONSTANT_BUFFER_VIEW_DESC>::value)
				{
					device->CreateConstantBufferView((const D3D12_CONSTANT_BUFFER_VIEW_DESC*)viewDesc,
													 viewHandle);
				}
				else if constexpr (std::is_same<viewDescType, D3D12_SHADER_RESOURCE_VIEW_DESC>::value)
				{
					device->CreateShaderResourceView(dataResrc.Get(),
													 (const D3D12_SHADER_RESOURCE_VIEW_DESC*)viewDesc,
													 viewHandle);
				}
				else if constexpr (std::is_same<viewDescType, D3D12_UNORDERED_ACCESS_VIEW_DESC>::value)
				{
					device->CreateUnorderedAccessView(dataResrc.Get(),
													  ctrResrc.Get(),
													  (const D3D12_UNORDERED_ACCESS_VIEW_DESC*)viewDesc,
													  viewHandle);
				}
				// Update descriptor-heap offset
				shaderViewMem.offs.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				return viewHandle;
			}

			// Abstraction for stack-like GPU memory chunks, defining an offset (zero-initialized integer for resources,
			// procedurally-initialized handle for descriptors) and a chunk type (resource heap/descriptor heap)
			template<typename HeapType> // Should limit to [ID3D12Heap]/[ID3D12DescriptorHeap] with C++20 concepts
			struct GPUStackedMem
			{
				GPUStackedMem() {}; // Empty constructor to simplify broader [GPUMemory] setup
				typedef typename std::conditional<std::is_same<HeapType, ID3D12Heap>::value,
												  u8Byte,
												  D3D12_CPU_DESCRIPTOR_HANDLE>::type offsType;
				typedef typename std::conditional<std::is_same<HeapType, ID3D12Heap>::value,
												  D3D12_HEAP_DESC,
												  D3D12_DESCRIPTOR_HEAP_DESC>::type memDescType;
				GPUStackedMem(const memDescType& memDesc, const Microsoft::WRL::ComPtr<ID3D12Device>& device)
				{
					// Initialize GPU memory
					constexpr bool resrcStack = std::is_same<HeapType, ID3D12Heap>::value;
					if constexpr (resrcStack) { assert(SUCCEEDED(device->CreateHeap(&memDesc, __uuidof(ID3D12Heap), (void**)mem.GetAddressOf()))); }
					else if constexpr (!resrcStack) { assert(SUCCEEDED(device->CreateDescriptorHeap(&memDesc, __uuidof(ID3D12DescriptorHeap), (void**)mem.GetAddressOf()))); }

					// Initialize data offset
					if constexpr (resrcStack)
					{ offs = 0; }
					else if constexpr (std::is_same<HeapType, ID3D12DescriptorHeap>::value)
					{ offs = mem->GetCPUDescriptorHandleForHeapStart(); }
				}
				offsType offs; // [GPUMemory] uses a similar stack-based model to [StackAllocator]
							   // Defined in bytes for [uploMem] and [resrcMem], multiples of cbv/uav/srv descriptor size for [shaderViewMem]
				Microsoft::WRL::ComPtr<HeapType> mem;
			};
			GPUStackedMem<ID3D12Heap> resrcMem; // Main buffer memory
			GPUStackedMem<ID3D12Heap> uploMem; // CPU->GPU intermediate upload memory (used for planetary load-in + cbuffers)
											   // Animal memory could easily be too dense to be wholly GPU-resident, so stream that in through tiled resources instead
            GPUStackedMem<ID3D12Heap> rdbkMem; // GPU->CPU intermediate readback memory (mainly used for tracking path counts between shading stages; Athru's path-tracing
                                               // style guarantees stalls between stages (either from UAV barriers or from readback), so there's less reason to choose indirect
                                               // dispatch over a conceptually simpler readback->dispatch->update->readback loop)
			GPUStackedMem<ID3D12DescriptorHeap> shaderViewMem; // Descriptor memory for shader-visible views (constant-buffer, shader-resource, unordered-access)
															   // Not necessary yet, but likely to have one descriptor heap for storage and smaller ones for per-context work
															   // (descriptors across contexts would be stored in [shaderViewMem], then those needed for specific contexts
															   //  would be copied across to [rnderViewMem], [physicsViewMem], and [ecoViewMem] each frame)
															   // This simplifies descriptor management, and makes it natural to only include the context bindings you'll use
															   // in each shader
			D3D12_GPU_DESCRIPTOR_HANDLE rnderCtxStart; // GPU handle to the start of the rendering descriptor heap
			// -- No render-targets or depth-stencils used by Athru, compute-shading only -- //
	};
}

