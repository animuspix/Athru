#include "UtilityServiceCentre.h"
#include "ComputePass.h"
#include <filesystem>

ComputePass::ComputePass(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
						 HWND windowHandle,
						 const char* shaderFilePath,
						 const u4Byte& numCBVs, const u4Byte& numSRVs, const u4Byte& numUAVs)
{
	// Import the given file into Athru with an ordinary C++ binary input/output stream
	std::ifstream strm = std::ifstream(shaderFilePath, std::ios::binary);
	u8Byte len = std::filesystem::file_size(shaderFilePath); // File size in bytes
	strm.read(dxilMem, len); // Read bytecode into the subset of Athru's memory stack associated with [this]

	// Compose descriptor ranges
	u4Byte numRanges = 3;
	D3D12_DESCRIPTOR_RANGE ranges[3];
	ranges[0].BaseShaderRegister = 0;
	ranges[0].NumDescriptors = numCBVs;
	ranges[0].OffsetInDescriptorsFromTableStart = 0;
	ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	ranges[0].RegisterSpace = 0;
	ranges[1] = ranges[0];
	ranges[1].OffsetInDescriptorsFromTableStart = numCBVs;
	ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[1].NumDescriptors = numSRVs;
	ranges[2] = ranges[0];
	ranges[2].OffsetInDescriptorsFromTableStart = numCBVs + numSRVs;
	ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	ranges[2].NumDescriptors = numUAVs;

	// No slot is allowed to bind zero descriptors, so shuffle ranges around appropriately
	if (numCBVs == 0) { memcpy(&ranges[0], &ranges[1], sizeof(D3D12_DESCRIPTOR_RANGE) * 2); numRanges -= 1; }
	if (numSRVs == 0) { memcpy(&ranges[numRanges - 2], &ranges[2], sizeof(D3D12_DESCRIPTOR_RANGE) * 2); numRanges -= 1; }
	if (numUAVs == 0) { /* No copies needed, UAVs are last in the descriptor range anyway */ numRanges -= 1; }

	// Prepare generated ranges into a table we can embed in the pass's PSO
	D3D12_ROOT_PARAMETER rootParam = { };
	rootParam.DescriptorTable.NumDescriptorRanges = numRanges; // Only SRVs/UAVs/CBVs to think about for compute
	rootParam.DescriptorTable.pDescriptorRanges = ranges;
	rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // No option for compute-only; all raster stages are manually screened below
	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	u4Byte numSamplers = 0;

	// Prepare root signature/resource context
	D3D12_ROOT_SIGNATURE_DESC sig = { };
	sig.NumParameters = 1; // All descriptors + constants are indirectly bound through a descriptor table (configured from the shading context)
	sig.pParameters = &rootParam;
	sig.pStaticSamplers = nullptr; // No samplers needed for compute atm, possibly some needed later; assuming none atm
	sig.NumStaticSamplers = numSamplers;
	sig.Flags = D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS; // Compute pipeline-state is only compatible with
																		 // compute shading stages (+ Athru is expected to be compute-only)
	Microsoft::WRL::ComPtr<ID3DBlob> sigBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> sigErrBlob;
	HRESULT result = D3D12SerializeRootSignature(&sig,
												 D3D_ROOT_SIGNATURE_VERSION::D3D_ROOT_SIGNATURE_VERSION_1_0,
												 &sigBlob,
												 &sigErrBlob); // Procedural root signature descriptions must be serialized before the
															   // signatures themselves can be created by the device
	#ifdef _DEBUG
		char* serializerErrors = (sigErrBlob == nullptr) ? nullptr : (char*)sigErrBlob->GetBufferPointer();
	#endif
	assert(SUCCEEDED(result));
	device->CreateRootSignature(0x1,
							    sigBlob->GetBufferPointer(),
								sigBlob->GetBufferSize(),
								__uuidof(ID3D12RootSignature),
								(void**)&rootSig);

	// Assemble pipeline state
	D3D12_COMPUTE_PIPELINE_STATE_DESC pipeDesc = { };
	pipeDesc.pRootSignature = rootSig.Get();
	pipeDesc.CS.pShaderBytecode = (void*)dxilMem;
	pipeDesc.CS.BytecodeLength = len;
	pipeDesc.NodeMask = 0x1; // Athru only uses one GPU atm
	result = device->CreateComputePipelineState(&pipeDesc, __uuidof(ID3D12PipelineState), (void**)&shadingState);
	assert(SUCCEEDED(result));
}

ComputePass::~ComputePass()
{
	rootSig = nullptr;
	shadingState = nullptr;
}

// Push constructions for this class through Athru's custom allocator
void* ComputePass::operator new(size_t size)
{
	StackAllocator* allocator = AthruCore::Utility::AccessMemory();
	return allocator->AlignedAlloc(size, (uByte)std::alignment_of<ComputePass>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void ComputePass::operator delete(void* target)
{
	return;
}
