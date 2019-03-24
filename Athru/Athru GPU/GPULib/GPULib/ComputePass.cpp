#include "UtilityServiceCentre.h"
#include "ComputePass.h"

ComputePass::ComputePass(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
						 HWND windowHandle,
						 const char* shaderFilePath,
					     const AthruGPU::RESRC_CTX shadingCtx)
{
	// Import the given file into Athru with an ordinary C++ binary input/output stream
	std::ifstream strm = std::ifstream(shaderFilePath, std::ios::binary);
	u4Byte len = 0; // File size in bytes
	while (strm.eof()) { strm.seekg(len), len += 1; } // Find the character (byte) length of the given shader
	strm.read(dxilMem, len); // Read bytecode into the subset of Athru's memory stack associated with [this]

	// Map shading context (rendering/generic, rendering/generic/physics, rendering/generic/physics/ecology) onto
	// descriptor ranges
	// -- need to update shaders for SM6 and review before implementing this --
	D3D12_ROOT_PARAMETER rootParam = { };
	rootParam.DescriptorTable.NumDescriptorRanges = 3; // Only SRVs/UAVs/CBVs to think about for compute
													   // Nothing guarantees srv/uav/cbv sets are contiguous (as they need to be for descriptor ranges iirc),
													   // should adjust [GPUMemory] to help with that
	rootParam.DescriptorTable.pDescriptorRanges = nullptr; // Dependant on shader context, locked to [nullptr] atm
	rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // No option for compute-only; all raster stages are manually screened below
	u4Byte numSamplers = 0; // Likely dependant on shader context, locked to zero atm

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
	assert(SUCCEEDED(result));
	device->CreateRootSignature(0x1,
							    (void*)sigBlob.Get(),
								sigBlob->GetBufferSize(),
								__uuidof(ID3D12RootSignature),
								(void**)&rootSig);

	// Assemble pipeline state
	D3D12_COMPUTE_PIPELINE_STATE_DESC pipeDesc = { };
	pipeDesc.pRootSignature;
	pipeDesc.CS.pShaderBytecode = (void*)dxilMem;
	pipeDesc.CS.BytecodeLength = len;
	pipeDesc.NodeMask = 0x1; // Athru only uses one GPU atm
	result = device->CreateComputePipelineState(&pipeDesc, __uuidof(ID3D12PipelineState), (void**)&shadingState);
	assert(SUCCEEDED(result));
}

ComputePass::~ComputePass()
{
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
