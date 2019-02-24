#include "GPUMemory.h"
#include "UtilityServiceCentre.h"

GPUMemory::GPUMemory(const Microsoft::WRL::ComPtr<ID3D12Device>& device)
{
	// Create main GPU heap
	D3D12_HEAP_DESC memDesc;
	memDesc.SizeInBytes = AthruGPU::EXPECTED_ONBOARD_GPU_RESRC_MEM;
	memDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM; // Prefer explicit paging information + memory-pool preference when possible
	memDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE; // No cpu access for main memory
	memDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_L1; // Games built with Athru will be demanding enough for implicitly requiring a discrete adapter
																	// to be ok
	memDesc.Properties.CreationNodeMask = 0x1; // Create main memory on the zeroth GPU captured by the adapter (we're only building a single-gpu/single-adapter system
											   // atm so this is guaranteed to be the discrete adapter)
	memDesc.Properties.VisibleNodeMask = 0x1; // Expecting to use per-GPU heaps (since the discrete/integrated GPUs will be doing very different things) when I extend
											  // Athru for multiple adapters
	memDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	memDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
	assert(SUCCEEDED(device->CreateHeap(&memDesc, __uuidof(ID3D12Heap), (void**)resrcMem.mem.GetAddressOf())));

	// Create the upload heap
	memDesc.SizeInBytes = AthruGPU::EXPECTED_SHARED_GPU_UPLO_MEM;
	memDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD; // Unsure what paging mode to select for cpu->gpu uploads; let the driver work things out instead :p
	memDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	memDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	memDesc.Properties.CreationNodeMask = 0x1; // Only one adapter atm
	memDesc.Properties.VisibleNodeMask = 0x1;
	// Alignment & flags are the same as main memory
	assert(SUCCEEDED(device->CreateHeap(&memDesc, __uuidof(ID3D12Heap), (void**)uploMem.mem.GetAddressOf())));

	// Create the shader-visible descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descHeapDesc.NumDescriptors = AthruGPU::EXPECTED_NUM_GPU_SHADER_VIEWS;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0x1; // Only one adapter atm
	assert(SUCCEEDED(device->CreateDescriptorHeap(&descHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)shaderViewMem.mem.GetAddressOf())));

	// -- No render-targets or depth-stencils used by Athru, compute-shading only -- //
}

GPUMemory::~GPUMemory()
{
	// Reset stack offsets
	resrcMem.offs = 0;
	shaderViewMem.offs = 0;

	// Clear GPU allocations here
	// --

	// Explicitly reset smart pointers
	resrcMem.mem = nullptr;
	shaderViewMem.mem = nullptr;
}

HRESULT GPUMemory::AllocBuf(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
							const u4Byte& bufSize,
							const D3D12_RESOURCE_DESC* bufDesc,
							const D3D12_RESOURCE_STATES initState,
							const Microsoft::WRL::ComPtr<ID3D12Resource>& bufPttr,
							const AthruGPU::HEAP_TYPES heap = AthruGPU::HEAP_TYPES::GPU_ACCESS_ONLY)
{
	GPUStackedMem<ID3D12Heap>& mem = (heap == AthruGPU::HEAP_TYPES::GPU_ACCESS_ONLY) ? resrcMem : uploMem;
	u4Byte offs = mem.offs;
	mem.offs += bufSize;
	return device->CreatePlacedResource(mem.mem.Get(),
										offs,
										bufDesc,
										initState,
										nullptr,
										__uuidof(ID3D12Resource),
										(void**)bufPttr.GetAddressOf());
}

HRESULT GPUMemory::AllocTex(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
							const u4Byte& texSize,
							const D3D12_RESOURCE_DESC* texDesc,
							const D3D12_RESOURCE_STATES initState,
							const D3D12_CLEAR_VALUE optimalClearColor,
							const Microsoft::WRL::ComPtr<ID3D12Resource>& texPttr)
{
	u4Byte offs = resrcMem.offs;
	resrcMem.offs += texSize;
	return device->CreatePlacedResource(resrcMem.mem.Get(),
										offs,
										texDesc,
										initState,
										&optimalClearColor,
										__uuidof(ID3D12Resource),
										(void**)texPttr.GetAddressOf());
}

// Push constructions for this class through Athru's custom allocator
void* GPUMemory::operator new(size_t size)
{
	StackAllocator* allocator = AthruCore::Utility::AccessMemory();
	return allocator->AlignedAlloc(size, (uByte)std::alignment_of<GPUMemory>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void GPUMemory::operator delete(void* target)
{
	return;
}
