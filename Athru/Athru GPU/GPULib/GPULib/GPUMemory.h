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
						 const AthruGPU::HEAP_TYPES = AthruGPU::HEAP_TYPES::GPU_ACCESS_ONLY); // Likely to start passing [AthruResrc]s here instead of generic resources when possible
		// [GPUMemory] expects aligned Buffer/Texture/View allocations
		HRESULT AllocTex(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
						 const u4Byte& bufSize,
						 const D3D12_RESOURCE_DESC* texDesc,
						 const D3D12_RESOURCE_STATES initState,
						 const D3D12_CLEAR_VALUE optimalClearColor,
						 const Microsoft::WRL::ComPtr<ID3D12Resource>& texPttr); // Likely to start passing [AthruTexture]s here instead of generic resources when possible
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

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);
	private:
		template<typename HeapType> // Should limit to [ID3D12Heap]/[ID3D12DescriptorHeap] with C++20 concepts
		struct GPUStackedMem
		{
			u8Byte offs = 0; // [GPUMemory] uses a similar stack-based model to [StackAllocator]
			Microsoft::WRL::ComPtr<HeapType> mem;
		};
		GPUStackedMem<ID3D12Heap> resrcMem; // Main buffer/texture/queue/list memory
		GPUStackedMem<ID3D12Heap> uploMem; // CPU->GPU intermediate upload memory (used for planetary load-in + cbuffers)
										   // Animal memory could easily be too dense to be wholly GPU-resident, so stream that in through tiled resources instead
		GPUStackedMem<ID3D12DescriptorHeap> shaderViewMem; // Descriptor memory for shader-visible views (constant-buffer, shader-resource, unordered-access)
		// -- No render-targets or depth-stencils used by Athru, compute-shading only -- //
};

