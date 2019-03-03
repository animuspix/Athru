#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include "GPUGlobals.h"

class GPUMemory
{
	public:
		GPUMemory(const Microsoft::WRL::ComPtr<ID3D12Device>& device);
		~GPUMemory();
		// [GPUMemory] expects aligned Buffer/Texture/View allocations
		HRESULT AllocBuf(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
						 const u4Byte& bufSize,
						 const D3D12_RESOURCE_DESC* bufDesc,
						 const D3D12_RESOURCE_STATES initState,
						 const Microsoft::WRL::ComPtr<ID3D12Resource>& bufPttr,
						 const AthruGPU::HEAP_TYPES = AthruGPU::HEAP_TYPES::GPU_ACCESS_ONLY);
		// [GPUMemory] expects aligned Buffer/Texture/View allocations
		HRESULT AllocTex(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
						 const u4Byte& bufSize,
						 const D3D12_RESOURCE_DESC* texDesc,
						 const D3D12_RESOURCE_STATES initState,
						 const D3D12_CLEAR_VALUE optimalClearColor,
						 const Microsoft::WRL::ComPtr<ID3D12Resource>& texPttr);
		template<typename BufType>
		void ArrayToGPUBuffer(const BufType* bufArr,
							  const u4Byte& size,
							  const D3D12_RESOURCE_DESC* bufDesc,
							  const D3D12_RESOURCE_STATES initState,
							  const Microsoft::WRL::ComPtr<ID3D12Device>& device,
							  const Microsoft::WRL::ComPtr<ID3D12Resource>& bufPttr)
		{
			// - Construct temporary upload heap/resource through [CreateCommittedResource]
			Microsoft::WRL::ComPtr<ID3D12Resource> uplo;
			D3D12_HEAP_PROPERTIES heapProps;
			heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapProps.VisibleNodeMask = 0x1;
			heapProps.CreationNodeMask = 0x1;
			HRESULT hr = device->CreateCommittedResource(heapProps,
														 D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
														 bufDesc,
														 initState,
														 nullptr,
														 __uuidof(ID3D12Resource),
														 (void**)uplo);
			assert(SUCCEEDED(hr));

			// - Populate with array data through [Map]
			BufType* uploStart;
			D3D12_RANGE readRange;
			readRange.Begin = 0;
			readRange.End = 0; // No readback during buffer upload
			assert(SUCCEEDED(uplo->Map(0, &readRange, (void**)uploStart)));
			memcpy(uploStart, bufArr, size);
			D3D12_RANGE writeRange;
			writeRange.Begin = 0;
			writeRange.End = size;
			assert(SUCCEEDED(uplo->Unmap(0, &writeRange))); // Not certain about [WrittenRange] here

			// - Create application-lifetime default resource within [resrcMem]
			u4Byte offs = resrcMem.offs;
			resrcMem.offs += bufSize;
			hr = device->CreatePlacedResource(resrcMem.mem.Get(),
											  offs,
											  bufDesc,
											  initState,
											  nullptr,
											  __uuidof(ID3D12Resource),
											  (void**)bufPttr.GetAddressOf());
			assert(SUCCEEDED(hr));

			// - Copy between the temporary upload heap/resource and [bufPttr] (allocated within [resrcMem])
			const Microsoft::WRL::ComPtr<ID3D12CommandList>& copyCmdList = AthruGPU::GPU::AccessD3D()->GetCopyCmdList();
			assert(SUCCEEDED(copyCmdList->Reset(AthruGPU::GPU::AccessD3D()->GetCopyCmdAllocator(),
												AthruGPU::GPU::AccessD3D()->GetComputeState().Get())));
			assert(SUCCEEDED(copyCmdList->CopyResource(bufPttr, uplo)));
			assert(SUCCEEDED(copyCmdList->Close()));
			assert(SUCCEEDED(AthruGPU::GPU::AccessD3D()->GetCopyCmdQueue()->ExecuteCommandLists(1, copyCmdList.GetAddressOf())));
			// Try to avert clearing the upload buffer before its data's moved into [bufPttr]
			AthruGPU::GPU::AccessD3D()->WaitForQueue<D3D12_COMMAND_LIST_TYPE_COPY>();

			// --Allow temporary data to release as it passes out of scope--
		}
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
			// Update descriptor-heap offset
			shaderViewMem.offs.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			// Cache view handle
			D3D12_CPU_DESCRIPTOR_HANDLE viewHandle;
			viewHandle.ptr = shaderViewMem.offs.ptr;

			// Allocate view & return descriptor handle
			if (std::is_same<viewDescType, D3D12_CONSTANT_BUFFER_VIEW_DESC>::value)
			{
				device->CreateConstantBufferView((const D3D12_CONSTANT_BUFFER_VIEW_DESC*)viewDesc,
												 viewHandle);
			}
			else if (std::is_same<viewDescType, D3D12_SHADER_RESOURCE_VIEW_DESC>::value)
			{
				device->CreateShaderResourceView(dataResrc.Get(),
												 (const D3D12_SHADER_RESOURCE_VIEW_DESC*)viewDesc,
												 viewHandle);
			}
			else if (std::is_same<viewDescType, D3D12_UNORDERED_ACCESS_VIEW_DESC>::value)
			{
				device->CreateUnorderedAccessView(dataResrc.Get(),
												  ctrResrc.Get(),
												  (const D3D12_UNORDERED_ACCESS_VIEW_DESC*)viewDesc,
												  viewHandle);
			}
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
				else if constexpr (resrcStack) { assert(SUCCEEDED(device->CreateDescriptorHeap(&memDesc, __uuidof(ID3D12DescriptorHeap), (void**)mem.GetAddressOf()))); }

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
		GPUStackedMem<ID3D12Heap> resrcMem; // Main buffer/texture/queue/list memory
		GPUStackedMem<ID3D12Heap> uploMem; // CPU->GPU intermediate upload memory (used for planetary load-in + cbuffers)
										   // Animal memory could easily be too dense to be wholly GPU-resident, so stream that in through tiled resources instead
		GPUStackedMem<ID3D12DescriptorHeap> shaderViewMem; // Descriptor memory for shader-visible views (constant-buffer, shader-resource, unordered-access)
		D3D12_CPU_DESCRIPTOR_HANDLE shaderViewStart; // Handle for the start of [shaderViewMem.mem]; not easily defined within [GPUStackedMem], not enough descriptor
													 // heaps to justify creating another struct
		// -- No render-targets or depth-stencils used by Athru, compute-shading only -- //
};

