#include "GPUServiceCentre.h"
#include "Renderer.h"
#include "PixHistory.h"
#include "Camera.h"
#include "GPUMemory.h"
#include <functional>

Renderer::Renderer(HWND windowHandle,
				   AthruGPU::GPUMemory& gpuMem,
				   const Microsoft::WRL::ComPtr<ID3D12Device>& device,
				   const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& rndrCmdQueue,
				   const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& rndrCmdList,
				   const Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& rndrCmdAlloc) :
			tracer{ ComputePass(device,
					 			windowHandle,
					 			"RayMarch.cso",
								AthruGPU::RESRC_CTX::RNDR_OR_GENERIC) },
			bouncePrep(device,
					   windowHandle,
					   "BouncePrep.cso",
					   AthruGPU::RESRC_CTX::RNDR_OR_GENERIC),
            lensSampler(ComputePass(device,
								    windowHandle,
								    "LensSampler.cso",
								    AthruGPU::RESRC_CTX::RNDR_OR_GENERIC)),
			bxdfs{ ComputePass(device,
							   windowHandle,
							   "DiffuSampler.cso",
							   AthruGPU::RESRC_CTX::RNDR_OR_GENERIC),
				   ComputePass(device,
				   		       windowHandle,
				   		       "MirroSampler.cso",
				   		       AthruGPU::RESRC_CTX::RNDR_OR_GENERIC),
				   ComputePass(device,
				   		       windowHandle,
				   		       "RefraSampler.cso",
				   		       AthruGPU::RESRC_CTX::RNDR_OR_GENERIC),
				   ComputePass(device,
				   		       windowHandle,
				   		       "SnowwSampler.cso",
				   		       AthruGPU::RESRC_CTX::RNDR_OR_GENERIC),
				   ComputePass(device,
				   		       windowHandle,
				   		       "SsurfSampler.cso",
				   		       AthruGPU::RESRC_CTX::RNDR_OR_GENERIC),
				   ComputePass(device,
				   		       windowHandle,
							   "FurrySampler.cso",
							   AthruGPU::RESRC_CTX::RNDR_OR_GENERIC) },
			post(device,
				 windowHandle,
				 "RasterPrep.cso",
				 AthruGPU::RESRC_CTX::RNDR_OR_GENERIC),
			dispatchScale128(device,
							 windowHandle,
							 "dispScale128.cso",
							 AthruGPU::RESRC_CTX::RNDR_OR_GENERIC),
			dispatchScale256(device,
							 windowHandle,
							 "dispScale256.cso",
							 AthruGPU::RESRC_CTX::RNDR_OR_GENERIC),
			dispatchScale512(device,
							 windowHandle,
							 "dispScale512.cso",
							 AthruGPU::RESRC_CTX::RNDR_OR_GENERIC),
			renderQueue(rndrCmdQueue),
			renderCmdList(rndrCmdList),
			renderCmdAllocator(rndrCmdAlloc)
{
    // Prepare D3D12 resources
    //////////////////////////

	// Build the buffer we'll be using to store
	// image samples for temporal smoothing/anti-aliasing
	aaBuffer.InitBuf(device,
					 gpuMem,
					 GraphicsStuff::DISPLAY_AREA);

	// Create counter buffers
	// Likely more efficient to define these as mostly-GPU and initialize on the zeroth frame than to upload
	// single-use initial data on startup
	for (u4Byte i = 0; i < 8; i += 1)
	{
		ctrs[i].InitBuf(device,
						gpuMem,
						3,
						DXGI_FORMAT::DXGI_FORMAT_R32_UINT);
	}

	// Create the tracing append/consume buffer
	traceables.InitAppBuf(device,
						  gpuMem,
						  nullptr,
						  GraphicsStuff::TILING_AREA,
						  0);

	// Create the intersection/surface buffer
	surfIsections.InitAppBuf(device,
							 gpuMem,
							 nullptr,
						     GraphicsStuff::TILING_AREA,
							 0);

	// Create the diffuse intersection buffer
	diffuIsections.InitAppBuf(device,
							  gpuMem,
							  nullptr,
						      GraphicsStuff::TILING_AREA,
							  0);

	// Create the mirrorlike intersection buffer
	mirroIsections.InitAppBuf(device,
							  gpuMem,
							  nullptr,
						      GraphicsStuff::TILING_AREA,
							  0);

	// Create the refractive intersection buffer
	refraIsections.InitAppBuf(device,
							  gpuMem,
							  nullptr,
						      GraphicsStuff::TILING_AREA,
							  0);

	// Create the snowy intersection buffer
	snowwIsections.InitAppBuf(device,
							  gpuMem,
							  nullptr,
						      GraphicsStuff::TILING_AREA,
							  36);

	// Create the organic/sub-surface scattering intersection buffer
	ssurfIsections.InitAppBuf(device,
							  gpuMem,
							  nullptr,
						      GraphicsStuff::TILING_AREA,
							  0);

	// Create the furry intersection buffer
	furryIsections.InitAppBuf(device,
							  gpuMem,
							  nullptr,
						      GraphicsStuff::TILING_AREA,
							  0);

	// Create the output texture
	displayTex.InitRWTex(device, gpuMem,
						 GraphicsStuff::DISPLAY_WIDTH,
						 GraphicsStuff::DISPLAY_HEIGHT);

	// Create specialized work submission/synchronization interfaces,
	// prepare rendering commands
	//////////////////////////

	// Create lens-sampling command list + allocator
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
								   __uuidof(lensAlloc),
								   (void**)&lensAlloc);
	device->CreateCommandList(0x1,
							  D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
							  lensAlloc.Get(),
							  lensSampler.shadingState.Get(),
							  __uuidof(lensList),
							  (void**)&lensList);

	// Most bindings should have already occurred during resource + pipeline-state setup
	// Prepare lens-sampling commands
	// Lens sampling in Athru/DX11 wrote to the counter buffer; no reason to do that in Athru/DX12
	// Also no reason for lens sampling to append elements into [traceables]; can just index path
	// data per-thread before tracing the zeroth bounce
	lensAlloc->Reset();
	lensList->SetDescriptorHeaps(1, gpuMem.GetShaderViewMem().GetAddressOf());
	DirectX::XMUINT3 disp = AthruGPU::tiledDispatchAxes(16);
	lensList->SetPipelineState(lensSampler.shadingState.Get());
	lensList->Dispatch(disp.x, disp.y, disp.z);
	lensList->Close(); // End lens-sampling submissions
	// -- No resource barrier here because we're timing PT start/stop to fence values anyway --

	// Create path-tracing command list + allocator
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
								   __uuidof(ptCmdAllocator),
								   (void**)&ptCmdAllocator);
	device->CreateCommandList(0x1,
							  D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
							  ptCmdAllocator.Get(),
							  tracer.shadingState.Get(),
							  __uuidof(ptCmdList),
							  (void**)&ptCmdList);

	// Prepare path-tracing commands
	ptCmdAllocator->Reset();
	ptCmdList->SetDescriptorHeaps(1, gpuMem.GetShaderViewMem().GetAddressOf());

	// Prepare indirect dispatch metadata
	D3D12_INDIRECT_ARGUMENT_DESC dispIndirArgDesc = {};
	dispIndirArgDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH; // No raster commands
	D3D12_COMMAND_SIGNATURE_DESC dispIndirSig = {};
	dispIndirSig.ByteStride = 12; // Unsigned integer arguments, three four-byte dispatch axes each
	dispIndirSig.NumArgumentDescs = 1; // Only one indirect argument type used for path tracing
	dispIndirSig.pArgumentDescs = &dispIndirArgDesc;
	dispIndirSig.NodeMask = 0x1;
	assert(SUCCEEDED(device->CreateCommandSignature(&dispIndirSig, nullptr, __uuidof(ptCmdSig), (void**)&ptCmdSig)));

	// Generate transition barriers for the counter buffers
	// Barrier order is tracing/bounce-prep/(diffuse/mirrorlike/refractive/snowy/generic-subsurface/furry)
	D3D12_RESOURCE_BARRIER indirBarriers[2];
	D3D12_RESOURCE_BARRIER writableBarriers[2];
	D3D12_RESOURCE_BARRIER indirBXDFBarriers[6];
	D3D12_RESOURCE_BARRIER writableBXDFBarriers[6];
	for (u4Byte i = 0; i < 8; i += 1)
	{
		D3D12_RESOURCE_STATES ctrState = ctrs[i].resrcState;
		if (i < 2)
		{
			indirBarriers[i] = AthruGPU::TransitionBarrier(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, ctrs[i].resrc.Get(), ctrState);
			writableBarriers[i] = AthruGPU::TransitionBarrier(ctrState, ctrs[i].resrc.Get(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		}
		else
		{
			u4Byte j = i - 2;
			indirBXDFBarriers[j] = AthruGPU::TransitionBarrier(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, ctrs[j].resrc.Get(), ctrState);
			writableBXDFBarriers[j] = AthruGPU::TransitionBarrier(ctrState, ctrs[j].resrc.Get(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		}
	}

	// Issue path-tracing work
	for (int i = 0; i < GraphicsStuff::MAX_NUM_BOUNCES; i += 1)
	{
		// Perform ray-tracing/marching
		ptCmdList->SetPipelineState(tracer.shadingState.Get());
		if (i > 0)
		{
			// Indirectly dispatch tracing work after the zeroth bounce
			ptCmdList->ExecuteIndirect(ptCmdSig.Get(), 1, ctrs[0].resrc.Get(), 0, nullptr, 0);
			ptCmdList->ResourceBarrier(1, &(writableBarriers[0]));
		}
		else
		{ ptCmdList->Dispatch(disp.x, disp.y, disp.z); } // Use a standard full-screen dispatch when counter buffers are uninitialized
		// Update indirect arguments for bounce preparation
		ptCmdList->ResourceBarrier(1, &(indirBarriers[1]));
		ptCmdList->ResourceBarrier(6, writableBXDFBarriers);
		ptCmdList->SetPipelineState(bouncePrep.shadingState.Get());
		ptCmdList->ExecuteIndirect(ptCmdSig.Get(), 1, ctrs[1].resrc.Get(), 0, nullptr, 0); // Prepare bounces
		// Update indirect arguments for surface sampling
		ptCmdList->ResourceBarrier(6, indirBXDFBarriers);
		ptCmdList->SetPipelineState(bxdfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::DIFFU].shadingState.Get());
		ptCmdList->ExecuteIndirect(ptCmdSig.Get(), 1, ctrs[2].resrc.Get(), 0, nullptr, 0); // Perform diffuse sampling (overlaps other sampling dispatches)
		ptCmdList->SetPipelineState(bxdfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::MIRRO].shadingState.Get());
		ptCmdList->ExecuteIndirect(ptCmdSig.Get(), 1, ctrs[3].resrc.Get(), 0, nullptr, 0); // Perform specular metallic sampling (overlaps other sampling dispatches)
		ptCmdList->SetPipelineState(bxdfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::REFRA].shadingState.Get());
		ptCmdList->ExecuteIndirect(ptCmdSig.Get(), 1, ctrs[4].resrc.Get(), 0, nullptr, 0); // Perform specular dielectric sampling (overlaps other sampling dispatches)
		ptCmdList->SetPipelineState(bxdfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::SNOWW].shadingState.Get());
		ptCmdList->ExecuteIndirect(ptCmdSig.Get(), 1, ctrs[5].resrc.Get(), 0, nullptr, 0); // Perform snow sampling (overlaps other sampling dispatches)
		ptCmdList->SetPipelineState(bxdfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::SSURF].shadingState.Get());
		ptCmdList->ExecuteIndirect(ptCmdSig.Get(), 1, ctrs[6].resrc.Get(), 0, nullptr, 0); // Perform generic subsurface sampling (overlaps other sampling dispatches)
		ptCmdList->SetPipelineState(bxdfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::FURRY].shadingState.Get());
		ptCmdList->ExecuteIndirect(ptCmdSig.Get(), 1, ctrs[7].resrc.Get(), 0, nullptr, 0); // Perform fur sampling (overlaps other sampling dispatches)
		// Update indirect arguments for ray-tracing/marching (tracing/marching pass is directly dispatched in the zeroth iteration)
		ptCmdList->ResourceBarrier(1, &(indirBarriers[0]));
		ptCmdList->ResourceBarrier(1, &(writableBarriers[1]));
	}
	D3D12_RESOURCE_BARRIER pathBarrier = AthruGPU::UAVBarrier(pxPaths.resrc);
	ptCmdList->ResourceBarrier(1, &pathBarrier); // Wait for path writes to finish before starting post-processing
	ptCmdList->Close(); // End PT submissions

	// Prepare post-processing commands
	renderCmdAllocator->Reset();
	renderCmdList->SetDescriptorHeaps(1, gpuMem.GetShaderViewMem().GetAddressOf());
	// -- No pixel resource barrier here, sampled colors are staged per-path and copied onto the display texture during post-processing --
	renderCmdList->SetPipelineState(post.shadingState.Get());
	renderCmdList->Dispatch(disp.x, disp.y, disp.z);

	// Prepare presentation commands
	////////////////////////////////

	// Copy the the display texture into the current back-buffer
	// Direct copies don't seem to work (can only access the zeroth back-buffer index?), should ask
	// CGSE about options here
	// Lots of stack-local logic here, might need to move those into persistent rendering state
	D3D12_TEXTURE_COPY_LOCATION texSrc;
	texSrc.pResource = displayTex.resrc.Get();
	texSrc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	texSrc.SubresourceIndex = 0; // No mip-maps on the display texture
	D3D12_TEXTURE_COPY_LOCATION texDst;
	Microsoft::WRL::ComPtr<ID3D12Resource> backBufTx; // Not sure about initialisation here
	texDst.pResource = backBufTx.Get();
	texDst.SubresourceIndex = 0; // No mip-maps on the back-buffer
	texDst.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

	// Prepare resource barriers
	D3D12_RESOURCE_BARRIER copyBarriers[2];
	copyBarriers[0] = AthruGPU::TransitionBarrier(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST,
												  backBufTx,
												  D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT);
	copyBarriers[1] = AthruGPU::TransitionBarrier(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE,
												  displayTex.resrc,
												  displayTex.resrcState);

	// Transition to copyable resources & perform copy to the back-buffer
	renderCmdList->ResourceBarrier(2, copyBarriers);
	renderCmdList->CopyTextureRegion(&texDst, 0, 0, 0, &texSrc, nullptr); // Perform copy to the back-buffer

	// Transition back to standard-use resources
	copyBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	copyBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT;
	copyBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE;
	copyBarriers[1].Transition.StateAfter = displayTex.resrcState;
	renderCmdList->ResourceBarrier(2, copyBarriers);
	renderCmdList->Close(); // End generic graphics submissions
	// -- Might need an extra sync here, unsure --
}

Renderer::~Renderer(){}

void Renderer::Render(Direct3D* d3d)
{
	// Perform lens sampling, path-tracing, and presentation
	ID3D12CommandList* cmdLists[3] = { lensList.Get(), ptCmdList.Get(), renderCmdList.Get() };
	renderQueue->ExecuteCommandLists(3, cmdLists);

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