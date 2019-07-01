#include "GPUMemory.h"
#include "GPUServiceCentre.h"
#include "UtilityServiceCentre.h"

AthruGPU::GPUMemory::GPUMemory(const Microsoft::WRL::ComPtr<ID3D12Device>& device)
{
	// Create main GPU heap
	D3D12_HEAP_DESC memDesc;
	memDesc.SizeInBytes = AthruGPU::EXPECTED_ONBOARD_GPU_RESRC_MEM;
	memDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;//D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM; // Prefer explicit paging information + memory-pool preference when possible
	memDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;//_NOT_AVAILABLE; // No cpu access for main memory
	memDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN; // Games built with Athru will be demanding enough for implicitly requiring a discrete adapter
																	     // to be ok
	memDesc.Properties.CreationNodeMask = 0x1; // Create main memory on the zeroth GPU captured by the adapter (we're only building a single-gpu/single-adapter system
											   // atm so this is guaranteed to be the discrete adapter)
	memDesc.Properties.VisibleNodeMask = 0x1; // Expecting to use per-GPU heaps (since the discrete/integrated GPUs will be doing very different things) when I extend
											  // Athru for multiple adapters
	memDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	memDesc.Flags = D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES;
	resrcMem = GPUStackedMem<ID3D12Heap>(memDesc, device);

	// Create the upload heap
	memDesc.SizeInBytes = AthruGPU::EXPECTED_SHARED_GPU_UPLO_MEM;
	memDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD; // Unsure what paging mode to select for cpu->gpu uploads; let the driver work things out instead :p
	memDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	memDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	memDesc.Properties.CreationNodeMask = 0x1; // Only one adapter atm
	memDesc.Properties.VisibleNodeMask = 0x1;
	// Alignment & flags are the same as main memory
	uploMem = GPUStackedMem<ID3D12Heap>(memDesc, device);

    // Create the readback heap
    memDesc.SizeInBytes = AthruGPU::EXPECTED_SHARED_GPU_RDBK_MEM;
    memDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK;
    memDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    memDesc.Properties.CreationNodeMask = 0x1; // Only one adapter atm
    memDesc.Properties.VisibleNodeMask = 0x1;
    // Alignment + flags are the same as main memory
    rdbkMem = GPUStackedMem<ID3D12Heap>(memDesc, device);

	// Create the shader-visible descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descHeapDesc.NumDescriptors = AthruGPU::EXPECTED_NUM_GPU_SHADER_VIEWS;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0x1; // Only one adapter atm
	shaderViewMem = GPUStackedMem<ID3D12DescriptorHeap>(descHeapDesc, device);
	rnderCtxStart = shaderViewMem.mem->GetGPUDescriptorHandleForHeapStart(); // Discrete per-context heaps are unimplemented atm

	// -- No render-targets or depth-stencils used by Athru, compute-shading only -- //
}

AthruGPU::GPUMemory::~GPUMemory()
{
	// Explicitly reset smart pointers
	resrcMem.mem = nullptr;
	uploMem.mem = nullptr;
	rdbkMem.mem = nullptr;
	shaderViewMem.mem = nullptr;
}

HRESULT AthruGPU::GPUMemory::AllocBuf(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
									  const u4Byte& bufSize,
									  const D3D12_RESOURCE_DESC bufDesc,
									  const D3D12_RESOURCE_STATES initState,
									  const Microsoft::WRL::ComPtr<ID3D12Resource>& bufPttr,
									  const AthruGPU::HEAP_TYPES heap)
{
	GPUStackedMem<ID3D12Heap>& mem = (heap == AthruGPU::HEAP_TYPES::GPU_ACCESS_ONLY) ? resrcMem : uploMem;
    if (heap == AthruGPU::HEAP_TYPES::READBACK) { mem = rdbkMem;}
	u8Byte offs = mem.offs;
	mem.offs += bufSize;
	return device->CreatePlacedResource(mem.mem.Get(),
										offs,
										&bufDesc,
										initState,
										nullptr,
										__uuidof(bufPttr),
										(void**)bufPttr.GetAddressOf());
}

HRESULT AthruGPU::GPUMemory::AllocTex(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
									  const u4Byte& texSize,
									  uByte* initDataPttr,
									  const D3D12_RESOURCE_DESC texDesc,
									  const D3D12_RESOURCE_STATES initState,
									  const Microsoft::WRL::ComPtr<ID3D12Resource>& texPttr)
{
	// Optimizing aligned memory usage for textures is very awkward, so prefer committed resources here
	///////////////////////////////////////////////////////////////////////////////////////////////////

	// Create target resource
	D3D12_HEAP_PROPERTIES heapProps;
	if (initDataPttr != nullptr)
	{ heapProps.Type = D3D12_HEAP_TYPE_CUSTOM; }
	else
	{ heapProps.Type = D3D12_HEAP_TYPE_DEFAULT; }
	heapProps.VisibleNodeMask = 0x1;
	heapProps.CreationNodeMask = 0x1;
	if (initDataPttr != nullptr)
	{ heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK; }
	else
	{ heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN; }
	if (initDataPttr != nullptr)
	{ heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_L0; }
	else
	{ heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN; }
	Microsoft::WRL::ComPtr<ID3D12Resource> uploPttr;
	HRESULT hr = device->CreateCommittedResource(&heapProps,
												 D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
												 &texDesc,
												 initState,
												 nullptr,
												 __uuidof(ID3D12Resource),
												 (void**)texPttr.GetAddressOf());
	assert(SUCCEEDED(hr));
	if (initDataPttr != nullptr)
	{
		u4Byte bytesPerPx = (u4Byte)(texSize / (texDesc.Width * texDesc.Height));
		hr = texPttr->WriteToSubresource(0, nullptr, initDataPttr, (u4Byte)(texDesc.Width * bytesPerPx), 0);
	}
	return hr;
}

D3D12_CPU_DESCRIPTOR_HANDLE AthruGPU::GPUMemory::AllocCBV(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
														  const D3D12_CONSTANT_BUFFER_VIEW_DESC* viewDesc)
{
	return AllocView(device,
					 viewDesc);
}

D3D12_CPU_DESCRIPTOR_HANDLE AthruGPU::GPUMemory::AllocSRV(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
														  const D3D12_SHADER_RESOURCE_VIEW_DESC* viewDesc,
														  const Microsoft::WRL::ComPtr<ID3D12Resource>& resrc)
{
	return AllocView(device,
					 viewDesc,
					 resrc);
}

D3D12_CPU_DESCRIPTOR_HANDLE AthruGPU::GPUMemory::AllocUAV(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
														  const D3D12_UNORDERED_ACCESS_VIEW_DESC* viewDesc,
														  const Microsoft::WRL::ComPtr<ID3D12Resource>& dataResrc,
														  const Microsoft::WRL::ComPtr<ID3D12Resource>& ctrResrc)
{
	return AllocView(device,
					 viewDesc,
					 dataResrc,
					 ctrResrc);
}

const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& AthruGPU::GPUMemory::GetShaderViewMem()
{
    return shaderViewMem.mem;
}

const D3D12_GPU_DESCRIPTOR_HANDLE AthruGPU::GPUMemory::GetBaseDescriptor(const AthruGPU::SHADER_CTX ctx)
{
	switch (ctx)
	{
		case AthruGPU::SHADER_CTX::RNDR:
			return rnderCtxStart;
		case AthruGPU::SHADER_CTX::PHYS:
			assert(false); // Unimplemented atm
			return rnderCtxStart; // Default to the rendering shader context
		case AthruGPU::SHADER_CTX::ECOL:
			assert(false); // Unimplemented atm
			return rnderCtxStart; // Default to the rendering shader context
		default:
			return rnderCtxStart; // Default to the rendering shader context
	}
}

// Push constructions for this class through Athru's custom allocator
void* AthruGPU::GPUMemory::operator new(size_t size)
{
	StackAllocator* allocator = AthruCore::Utility::AccessMemory();
	return allocator->AlignedAlloc(size, (uByte)std::alignment_of<GPUMemory>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void AthruGPU::GPUMemory::operator delete(void* target)
{
	return;
}
