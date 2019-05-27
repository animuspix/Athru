#include "GPUServiceCentre.h"
#include "Renderer.h"
#include "PixHistory.h"
#include "Camera.h"
#include "GPUMemory.h"
#include <array>
#include <functional>

Renderer::Renderer(HWND windowHandle,
				   AthruGPU::GPUMemory& gpuMem,
				   const Microsoft::WRL::ComPtr<ID3D12Device>& device,
				   const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& rndrCmdQueue,
				   const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& rndrCmdList,
				   const Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& rndrCmdAlloc) :
			// Initialize rendering code
			tracer{ ComputePass(device,
					 			windowHandle,
					 			"RayMarch.cso",
								AthruGPU::RESRC_CTX::RENDER,
								2, 2, 17) },
            lensSampler(ComputePass(device,
								    windowHandle,
								    "LensSampler.cso",
								    AthruGPU::RESRC_CTX::RENDER,
									2, 2, 10)),
			bxdfs{ ComputePass(device,
							   windowHandle,
							   "DiffuSampler.cso",
							   AthruGPU::RESRC_CTX::RENDER,
							   2, 2, 17),
				   ComputePass(device,
							   windowHandle,
							   "MirroSampler.cso",
							   AthruGPU::RESRC_CTX::RENDER,
							   2, 2, 17),
				   ComputePass(device,
							   windowHandle,
							   "RefraSampler.cso",
							   AthruGPU::RESRC_CTX::RENDER,
							   2, 2, 17),
				   ComputePass(device,
							   windowHandle,
							   "SnowwSampler.cso",
							   AthruGPU::RESRC_CTX::RENDER,
							   2, 2, 17),
				   ComputePass(device,
							   windowHandle,
							   "SsurfSampler.cso",
							   AthruGPU::RESRC_CTX::RENDER,
							   2, 2, 17),
				   ComputePass(device,
							   windowHandle,
							   "FurrySampler.cso",
							   AthruGPU::RESRC_CTX::RENDER,
							   2, 2, 17) },
			post(device,
				 windowHandle,
				 "RasterPrep.cso",
				 AthruGPU::RESRC_CTX::RENDER,
				 2, 0, 9),
			// Cache references to the rendering command queue/list/allocator
			renderQueue(rndrCmdQueue),
			renderCmdList(rndrCmdList),
			renderCmdAllocator(rndrCmdAlloc)
{
	// Initialize rendering buffer
	constexpr u4Byte sizes[15] = { (AthruGPU::NUM_RAND_PT_STREAMS * sizeof(PhiloStrm)),
								   (GraphicsStuff::DISPLAY_AREA * sizeof(float2x3)),
								   (GraphicsStuff::DISPLAY_AREA * sizeof(DirectX::XMFLOAT3)),
								   (GraphicsStuff::DISPLAY_AREA * sizeof(DirectX::XMFLOAT3)),
								   (GraphicsStuff::DISPLAY_AREA * sizeof(float)),
								   (GraphicsStuff::DISPLAY_AREA * sizeof(u4Byte)),
								   (GraphicsStuff::DISPLAY_AREA * sizeof(PixHistory)),
								   7 * 4096, // Counters are 4096-byte aligned; space between each counter is used for frame metadata
								   (GraphicsStuff::TILING_AREA * sizeof(u4Byte)),
								   (GraphicsStuff::TILING_AREA * sizeof(u4Byte)),
								   (GraphicsStuff::TILING_AREA * sizeof(u4Byte)),
								   (GraphicsStuff::TILING_AREA * sizeof(u4Byte)),
								   (GraphicsStuff::TILING_AREA * sizeof(u4Byte)),
								   (GraphicsStuff::TILING_AREA * sizeof(u4Byte)),
								   (GraphicsStuff::TILING_AREA * sizeof(u4Byte)) };
	u4Byte rndrMem = 0;
	for (u4Byte i = 0; i < 15; i += 1)
	{ rndrMem += sizes[i]; }
	rndrMem += 65536 - (rndrMem % 65536);
	rndrBuff.InitRawRWBuf(device, gpuMem, rndrMem);
	u4Byte offsets[15];
	offsets[0] = 0;
	offsets[1] = sizes[0];
	for (u4Byte i = 2; i < 15; i += 1)
	{ offsets[i] = offsets[i - 1] + sizes[i - 1]; }
	counterOffset = offsets[7] + (4096 - offsets[7] % 4096);
	randState.InitRWBufProj(device, gpuMem, rndrBuff.resrc, AthruGPU::NUM_RAND_PT_STREAMS);
	rays.InitRWBufProj(device, gpuMem, rndrBuff.resrc, GraphicsStuff::DISPLAY_AREA, sizes[0]);
	rayOris.InitRWBufProj(device, gpuMem, rndrBuff.resrc, GraphicsStuff::DISPLAY_AREA, offsets[2]);
	outDirs.InitRWBufProj(device, gpuMem, rndrBuff.resrc, GraphicsStuff::DISPLAY_AREA, offsets[3]);
	iors.InitRWBufProj(device, gpuMem, rndrBuff.resrc, GraphicsStuff::DISPLAY_AREA, offsets[4]);
	figIDs.InitRWBufProj(device, gpuMem, rndrBuff.resrc, GraphicsStuff::DISPLAY_AREA, offsets[5], DXGI_FORMAT_R32_UINT);
	displayTexHDR.InitRWTex(device, gpuMem, GraphicsStuff::DISPLAY_WIDTH, GraphicsStuff::DISPLAY_HEIGHT); // Initialize the display texture here so UAV layout
																									      // matches shader code
	displayTexLDR.InitRWTex(device, gpuMem, GraphicsStuff::DISPLAY_WIDTH, GraphicsStuff::DISPLAY_HEIGHT, 1u, DXGI_FORMAT_R8G8B8A8_UNORM);
	aaBuffer.InitRWBufProj(device, gpuMem, rndrBuff.resrc,  GraphicsStuff::DISPLAY_AREA, offsets[6]);
	counters.InitRWBufProj(device, gpuMem, rndrBuff.resrc, 7, offsets[7], DXGI_FORMAT_R32_UINT);
	traceables.InitAppProj(device, gpuMem, rndrBuff.resrc, rndrBuff.resrc, GraphicsStuff::TILING_AREA, counterOffset, offsets[8]);
	diffuIsections.InitAppProj(device, gpuMem, rndrBuff.resrc, rndrBuff.resrc, GraphicsStuff::TILING_AREA, counterOffset + 4096, offsets[9]);
	mirroIsections.InitAppProj(device, gpuMem, rndrBuff.resrc, rndrBuff.resrc, GraphicsStuff::TILING_AREA, counterOffset + 8192, offsets[10]);
	refraIsections.InitAppProj(device, gpuMem, rndrBuff.resrc, rndrBuff.resrc, GraphicsStuff::TILING_AREA, counterOffset + 12288, offsets[11]);
	snowwIsections.InitAppProj(device, gpuMem, rndrBuff.resrc, rndrBuff.resrc, GraphicsStuff::TILING_AREA, counterOffset + 16384, offsets[12]);
	ssurfIsections.InitAppProj(device, gpuMem, rndrBuff.resrc, rndrBuff.resrc, GraphicsStuff::TILING_AREA, counterOffset + 20480, offsets[13]);
	furryIsections.InitAppProj(device, gpuMem, rndrBuff.resrc, rndrBuff.resrc, GraphicsStuff::TILING_AREA, counterOffset + 24576, offsets[14]);

	// Create specialized work submission/synchronization interfaces,
	// prepare rendering commands
	//////////////////////////

	// Create lens-sampling command list + allocator
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
								   __uuidof(lensAlloc),
								   (void**)&lensAlloc);
	lensAlloc->Reset();
	device->CreateCommandList(0x1,
							  D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
							  lensAlloc.Get(),
							  lensSampler.shadingState.Get(),
							  __uuidof(lensList),
							  (void**)&lensList);

	// Prepare lens-sampling commands
	// Lens sampling in Athru/DX11 wrote to the counter buffer; no reason to do that in Athru/DX12
	// Also no reason for lens sampling to append elements into [traceables]; can just index path
	// data per-thread before tracing the zeroth bounce
	lensList->SetDescriptorHeaps(1, gpuMem.GetShaderViewMem().GetAddressOf());
	DirectX::XMUINT3 disp = AthruGPU::tiledDispatchAxes(16);
	lensList->SetComputeRootSignature(lensSampler.rootSig.Get());
	lensList->SetPipelineState(lensSampler.shadingState.Get());
	lensList->Dispatch(disp.x, disp.y, disp.z);
	lensList->Close(); // End lens-sampling submissions

	// Create path-tracing command lists + allocators
	// Path-tracing commands depend on per-frame information (dispatch counters), so they're recorded regularly
	// instead of once at startup
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
								   __uuidof(traceCmdAllocator),
								   (void**)&traceCmdAllocator);
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
								   __uuidof(surfCmdAllocator),
								   (void**)&surfCmdAllocator);

	// PT commands are reset per-bounce, no need for an initial reset here
	device->CreateCommandList(0x1,
							  D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
							  traceCmdAllocator.Get(),
							  tracer.shadingState.Get(),
							  __uuidof(traceCmdList),
							  (void**)&traceCmdList);
	device->CreateCommandList(0x1,
							  D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
							  surfCmdAllocator.Get(),
							  bxdfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::DIFFU].shadingState.Get(),
							  __uuidof(surfCmdList),
							  (void**)&surfCmdList);

	// Prepare post-processing commands
	renderCmdList->SetDescriptorHeaps(1, gpuMem.GetShaderViewMem().GetAddressOf());
	// -- No pixel resource barrier here, sampled colors are staged per-path and copied onto the display texture during post-processing --
	renderCmdList->SetComputeRootSignature(post.rootSig.Get());
	renderCmdList->SetPipelineState(post.shadingState.Get());
	renderCmdList->Dispatch(disp.x, disp.y, disp.z);

	// Prepare presentation commands
	////////////////////////////////

	// Copy the the display texture into the current back-buffer
	// Direct copies don't seem to work (can only access the zeroth back-buffer index?), should ask
	// CGSE about options here
	// Lots of stack-local logic here, might need to move those into persistent rendering state
	Direct3D* d3d = AthruGPU::GPU::AccessD3D();
	D3D12_TEXTURE_COPY_LOCATION texSrc;
	texSrc.pResource = displayTexLDR.resrc.Get();
	texSrc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	texSrc.SubresourceIndex = 0; // No mip-maps on the display texture
	D3D12_TEXTURE_COPY_LOCATION texDst;
	Microsoft::WRL::ComPtr<ID3D12Resource> backBufTx;
	d3d->GetBackBuf(backBufTx);
	texDst.pResource = backBufTx.Get();
	texDst.SubresourceIndex = 0; // No mip-maps on the back-buffer
	texDst.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

	// Prepare resource barriers
	D3D12_RESOURCE_BARRIER copyBarriers[2];
	copyBarriers[0] = AthruGPU::TransitionBarrier(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST,
												  backBufTx,
												  D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT);
	copyBarriers[1] = AthruGPU::TransitionBarrier(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE,
												  displayTexLDR.resrc,
												  displayTexLDR.resrcState);

	// Transition to copyable resources & perform copy to the back-buffer
	renderCmdList->ResourceBarrier(2, copyBarriers);
	renderCmdList->CopyTextureRegion(&texDst, 0, 0, 0, &texSrc, nullptr); // Perform copy to the back-buffer

	// Transition back to standard-use resources
	copyBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	copyBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT;
	copyBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE;
	copyBarriers[1].Transition.StateAfter = displayTexLDR.resrcState;
	renderCmdList->ResourceBarrier(2, copyBarriers);
	renderCmdList->Close(); // End generic graphics submissions
}

Renderer::~Renderer(){}

void Renderer::Render(Direct3D* d3d)
{
	// Perform lens-sampling
	renderQueue->ExecuteCommandLists(1, (ID3D12CommandList**)lensList.GetAddressOf());

	// Perform path-tracing
	///////////////////////

	// Access per-bounce counter data by caching an integer pointer to the readback buffer
	GPUMessenger* gpuMsg = AthruGPU::GPU::AccessGPUMessenger();

	// Prepare counter copy barriers (needed for before/after outputting data to the readback buffer)
	D3D12_RESOURCE_BARRIER ctrBarriers[2] = { AthruGPU::TransitionBarrier(D3D12_RESOURCE_STATE_COPY_SOURCE, rndrBuff.resrc.Get(), rndrBuff.resrcState),
											  AthruGPU::TransitionBarrier(rndrBuff.resrcState, rndrBuff.resrc.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE) };

	// Cache a reference to the readback buffer resource
	const Microsoft::WRL::ComPtr<ID3D12Resource> rdbkResrc = gpuMsg->AccessReadbackBuf();

	// Prepare command-list array for bounce staging
	ID3D12CommandList* bounceCmds[2] = { traceCmdList.Get(), surfCmdList.Get() };

	// Prepare path-tracing commands
	traceCmdList->SetPipelineState(tracer.shadingState.Get());
	for (int i = 0; i < GraphicsStuff::MAX_NUM_BOUNCES; i += 1)
	{
		// Perform ray-tracing/marching
		traceCmdList->SetDescriptorHeaps(1, d3d->GetGPUMem().GetShaderViewMem().GetAddressOf());
		traceCmdList->SetComputeRootSignature(tracer.rootSig.Get());
		if (i > 0)
		{
			// Scale tracing dispatches by remaining paths after the zeroth bounce
			u4Byte rays = gpuMsg->DataFromGPU<u4Byte>(0);
			traceCmdList->Dispatch(rays / 256, 1, 1); // Linear dispatch, simpler and more precise than multi-dimensional
		}
		else
		{
			// Use a standard full-screen dispatch when counters are uninitialized
			traceCmdList->Dispatch(GraphicsStuff::TILING_AREA, 1, 1);
		}

		// Update CPU-side sampler counters
		traceCmdList->ResourceBarrier(1, ctrBarriers); // Consider issuing both elements and using split barriers here
		traceCmdList->CopyBufferRegion(rdbkResrc.Get(), 4, rndrBuff.resrc.Get(), counterOffset, 24);
		traceCmdList->ResourceBarrier(1, &(ctrBarriers[1]));
		traceCmdList->Close();
		renderQueue->ExecuteCommandLists(1, (ID3D12CommandList**)traceCmdList.GetAddressOf()); // Submit tracing work to the graphics queue
		traceCmdList->Reset(traceCmdAllocator.Get(), tracer.shadingState.Get());

		// Wait for tracing commands to execute before copying out material counters
		d3d->WaitForQueue<D3D12_COMMAND_LIST_TYPE_DIRECT>();

		// Perform surface sampling
		surfCmdList->SetDescriptorHeaps(1, d3d->GetGPUMem().GetShaderViewMem().GetAddressOf());
		std::array<u4Byte, 6> ctrs;
		gpuMsg->DataFromGPU<decltype(ctrs)>(4);
		if (i == 0) { surfCmdList->SetPipelineState(bxdfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::DIFFU].shadingState.Get()); }
		surfCmdList->SetComputeRootSignature(bxdfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::DIFFU].rootSig.Get());
		surfCmdList->Dispatch(ctrs[0] / 256, 1, 1); // Perform diffuse sampling (overlaps other sampling dispatches)
		surfCmdList->SetPipelineState(bxdfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::MIRRO].shadingState.Get());
		surfCmdList->SetComputeRootSignature(bxdfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::MIRRO].rootSig.Get());
		surfCmdList->Dispatch(ctrs[1] / 256, 1, 1); // Perform specular metallic sampling (overlaps other sampling dispatches)
		surfCmdList->SetPipelineState(bxdfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::REFRA].shadingState.Get());
		surfCmdList->SetComputeRootSignature(bxdfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::REFRA].rootSig.Get());
		surfCmdList->Dispatch(ctrs[2] / 256, 1, 1); // Perform specular dielectric sampling (overlaps other sampling dispatches)
		surfCmdList->SetPipelineState(bxdfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::SNOWW].shadingState.Get());
		surfCmdList->SetComputeRootSignature(bxdfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::SNOWW].rootSig.Get());
		surfCmdList->Dispatch(ctrs[3] / 256, 1, 1); // Perform snow sampling (overlaps other sampling dispatches)
		surfCmdList->SetPipelineState(bxdfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::SSURF].shadingState.Get());
		surfCmdList->SetComputeRootSignature(bxdfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::SSURF].rootSig.Get());
		surfCmdList->Dispatch(ctrs[4] / 256, 1, 1); // Perform generic subsurface sampling (overlaps other sampling dispatches)
		surfCmdList->SetPipelineState(bxdfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::FURRY].shadingState.Get());
		surfCmdList->SetComputeRootSignature(bxdfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::FURRY].rootSig.Get());
		surfCmdList->Dispatch(ctrs[5] / 256, 1, 1); // Perform fur sampling (overlaps other sampling dispatches)

		// Update ray-tracing/marching counters (tracing/marching occurs as a full-screen pass in the zeroth iteration)
		surfCmdList->ResourceBarrier(1, ctrBarriers); // Consider issuing both elements and using split barriers here
		surfCmdList->CopyBufferRegion(rdbkResrc.Get(), 0, rndrBuff.resrc.Get(), ((u8Byte)counterOffset) + 24, 4);
		surfCmdList->ResourceBarrier(1, &(ctrBarriers[1]));
		surfCmdList->Close();
		renderQueue->ExecuteCommandLists(1, (ID3D12CommandList**)surfCmdList.GetAddressOf()); // Submit sampling work to the graphics queue
		surfCmdList->Reset(surfCmdAllocator.Get(), bxdfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::DIFFU].shadingState.Get());

		// Wait for surface sampling to finish before starting on tracing
		d3d->WaitForQueue<D3D12_COMMAND_LIST_TYPE_DIRECT>();
	}

	// Perform post-processing
	renderQueue->ExecuteCommandLists(1, (ID3D12CommandList**)renderCmdList.GetAddressOf());

	// Publish traced results to the display
	// No UAV barrier, because presentation includes a transition barrier that should cause a similar wait
	d3d->Present();
}

// Push constructions for this class through Athru's custom allocator
void* Renderer::operator new(size_t size)
{
	StackAllocator* allocator = AthruCore::Utility::AccessMemory();
	return allocator->AlignedAlloc(size, (uByte)std::alignment_of<Renderer>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void Renderer::operator delete(void* target)
{
	return;
}