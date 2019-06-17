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
				   const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& rndrCmdQueue) :
			// Initialize shaders
            lensSampler(ComputePass(device,
								    windowHandle,
								    "LensSampler.cso",
									1, 1, 18)),
			tracer{ ComputePass(device,
					 			windowHandle,
					 			"RayMarch.cso",
								1, 1, 24) },
			dispEditor { ComputePass(device,
									 windowHandle,
									 "RnderDispEditor.cso",
									 1, 1, 24) },
			surfSampler{ ComputePass(device,
								     windowHandle,
								     "SurfSampler.cso",
								     1, 1, 24) },
			post(device,
				 windowHandle,
				 "RasterPrep.cso",
				 1, 1, 17),
			// Cache a reference to the rendering command queue
			rnderQueue(rndrCmdQueue),
			rnderFrameCtr(0) // Rendering starts on the zeroth back-buffer
{
	// Initialize rendering buffer
	constexpr u4Byte sizes[15] = { (AthruGPU::NUM_RAND_PT_STREAMS * sizeof(PhiloStrm)),
								   (GraphicsStuff::DISPLAY_AREA * sizeof(float2x3)),
								   (GraphicsStuff::DISPLAY_AREA * sizeof(DirectX::XMFLOAT3)),
								   (GraphicsStuff::DISPLAY_AREA * sizeof(DirectX::XMFLOAT3)),
								   (GraphicsStuff::DISPLAY_AREA * sizeof(float)),
								   (GraphicsStuff::DISPLAY_AREA * sizeof(u4Byte)),
								   (GraphicsStuff::DISPLAY_AREA * sizeof(PixHistory)),
								   (8 * sizeof(u4Byte)),
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
	rndrMem += 65536 - (rndrMem % 65536); // Adjust rendering memory footprint toward 64KB alignment
	rndrBuff.InitRawRWBuf(device, gpuMem, rndrMem);
	u4Byte offsets[15];
	offsets[0] = 0;
	offsets[1] = sizes[0];
	for (u4Byte i = 2; i < 15; i += 1)
	{ offsets[i] = offsets[i - 1] + sizes[i - 1]; }
	randState.InitRWBufProj(device, gpuMem, rndrBuff.resrc, AthruGPU::NUM_RAND_PT_STREAMS);
	rays.InitRWBufProj(device, gpuMem, rndrBuff.resrc, GraphicsStuff::DISPLAY_AREA, offsets[1]);
	rayOris.InitRWBufProj(device, gpuMem, rndrBuff.resrc, GraphicsStuff::DISPLAY_AREA, offsets[2]);
	outDirs.InitRWBufProj(device, gpuMem, rndrBuff.resrc, GraphicsStuff::DISPLAY_AREA, offsets[3]);
	iors.InitRWBufProj(device, gpuMem, rndrBuff.resrc, GraphicsStuff::DISPLAY_AREA, offsets[4]);
	figIDs.InitRWBufProj(device, gpuMem, rndrBuff.resrc, GraphicsStuff::DISPLAY_AREA, offsets[5], DXGI_FORMAT_R32_UINT);
	displayTexHDR.InitRWTex(device, gpuMem, GraphicsStuff::DISPLAY_WIDTH, GraphicsStuff::DISPLAY_HEIGHT); // Initialize the display texture here so UAV layout
																										  // matches shader code
	displayTexLDR.InitRWTex(device, gpuMem, GraphicsStuff::DISPLAY_WIDTH, GraphicsStuff::DISPLAY_HEIGHT, 1u, DXGI_FORMAT_R8G8B8A8_UNORM);
	aaBuffer.InitRWBufProj(device, gpuMem, rndrBuff.resrc, GraphicsStuff::DISPLAY_AREA, offsets[6]);
	dispAxesWrite.InitRWBufProj(device, gpuMem, rndrBuff.resrc, 8, offsets[7], DXGI_FORMAT_R32_UINT);
	dispAxesArgs.InitRawRWBuf(device, gpuMem, AthruGPU::MINIMAL_D3D12_ALIGNED_BUFFER_MEM); // Allocate aligned argument buffer separately to main rendering data
																						   // so we can keep rendering data as [D3D12_RESOURCE_STATES_UNORDERED_ACCESS]
																						   // while dispatch arguments transition to [D3D12_RESOURCE_STATES_INDIRECT_ARGUMENT]
	ctrBuff.InitRawRWBuf(device, gpuMem, 7 * 4096); // Allocate material counter buffer separately to [rndrBuff] so we can pack data (counters are 4096-byte aligned) without
													// aliasing tracing/sampling indices or creating inaccessible memory between buffer projections
	traceCtr.InitRWBufProj(device, gpuMem, ctrBuff.resrc.Get(), 1, 0, DXGI_FORMAT_R32_UINT);
	diffuCtr.InitRWBufProj(device, gpuMem, ctrBuff.resrc.Get(), 1, 4096, DXGI_FORMAT_R32_UINT);
	mirroCtr.InitRWBufProj(device, gpuMem, ctrBuff.resrc.Get(), 1, 8192, DXGI_FORMAT_R32_UINT);
	refraCtr.InitRWBufProj(device, gpuMem, ctrBuff.resrc.Get(), 1, 12288, DXGI_FORMAT_R32_UINT);
	snowwCtr.InitRWBufProj(device, gpuMem, ctrBuff.resrc.Get(), 1, 16384, DXGI_FORMAT_R32_UINT);
	ssurfCtr.InitRWBufProj(device, gpuMem, ctrBuff.resrc.Get(), 1, 20480, DXGI_FORMAT_R32_UINT);
	furryCtr.InitRWBufProj(device, gpuMem, ctrBuff.resrc.Get(), 1, 24576, DXGI_FORMAT_R32_UINT);
	traceables.InitAppProj(device, gpuMem, rndrBuff.resrc, ctrBuff.resrc, GraphicsStuff::TILING_AREA, 0, offsets[8]);
	diffuIsections.InitAppProj(device, gpuMem, rndrBuff.resrc, ctrBuff.resrc, GraphicsStuff::TILING_AREA, 4096, offsets[9]);
	mirroIsections.InitAppProj(device, gpuMem, rndrBuff.resrc, ctrBuff.resrc, GraphicsStuff::TILING_AREA, 8192, offsets[10]);
	refraIsections.InitAppProj(device, gpuMem, rndrBuff.resrc, ctrBuff.resrc, GraphicsStuff::TILING_AREA, 12288, offsets[11]);
	snowwIsections.InitAppProj(device, gpuMem, rndrBuff.resrc, ctrBuff.resrc, GraphicsStuff::TILING_AREA, 16384, offsets[12]);
	ssurfIsections.InitAppProj(device, gpuMem, rndrBuff.resrc, ctrBuff.resrc, GraphicsStuff::TILING_AREA, 20480, offsets[13]);
	furryIsections.InitAppProj(device, gpuMem, rndrBuff.resrc, ctrBuff.resrc, GraphicsStuff::TILING_AREA, 24576, offsets[14]);

	// Create specialized work submission/synchronization interfaces,
	// prepare rendering commands
	//////////////////////////

	// Create shared rendering allocator
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
								   __uuidof(rnderAlloc),
								   (void**)&rnderAlloc);

	// Prepare lens-sampling commands
	device->CreateCommandList(0x1,
							  D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
							  rnderAlloc.Get(),
							  lensSampler.shadingState.Get(),
							  __uuidof(lensList),
							  (void**)& lensList);
	const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> viewHeap = gpuMem.GetShaderViewMem();
	lensList->SetDescriptorHeaps(1, viewHeap.GetAddressOf());
	lensList->SetComputeRootSignature(lensSampler.rootSig.Get());
	D3D12_GPU_DESCRIPTOR_HANDLE baseRndrDescriptor = gpuMem.GetBaseDescriptor(AthruGPU::SHADER_CTX::RNDR);
	lensList->SetComputeRootDescriptorTable(0, baseRndrDescriptor);
	lensList->SetPipelineState(lensSampler.shadingState.Get());
	DirectX::XMUINT3 disp = AthruGPU::tiledDispatchAxes(16);
	lensList->Dispatch(disp.x, disp.y, disp.z);
	lensList->Close(); // End lens-sampling submissions

	// Record path-tracing commands
	///////////////////////////////

	// Create path-tracing command list
	device->CreateCommandList(0x1,
							  D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
							  rnderAlloc.Get(),
							  tracer.shadingState.Get(),
							  __uuidof(ptCmdList),
							  (void**)&ptCmdList);

	// Prepare command signature for indirect dispatch
	D3D12_INDIRECT_ARGUMENT_DESC indirArgDesc;
	indirArgDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
	D3D12_COMMAND_SIGNATURE_DESC cmdSigDesc;
	cmdSigDesc.ByteStride = 24; // Three four-byte arguments
	cmdSigDesc.NumArgumentDescs = 1; // Only one argument type supported
	cmdSigDesc.pArgumentDescs = &indirArgDesc;
	cmdSigDesc.NodeMask = 0x1; // No support for multi-GPU atm
	device->CreateCommandSignature(&cmdSigDesc, nullptr, __uuidof(ptCmdSignature), &ptCmdSignature);

	// Prepare copy barriers for the read/write & indirect argument dispatch axes
	D3D12_RESOURCE_BARRIER dispBarriers[4] = { AthruGPU::TransitionBarrier(D3D12_RESOURCE_STATE_COPY_SOURCE, rndrBuff.resrc.Get(), rndrBuff.resrcState),
											   AthruGPU::TransitionBarrier(dispAxesArgs.resrcState, dispAxesArgs.resrc.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
											   AthruGPU::TransitionBarrier(rndrBuff.resrcState, rndrBuff.resrc.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE),
											   AthruGPU::TransitionBarrier(D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, dispAxesArgs.resrc.Get(), dispAxesArgs.resrcState) };

	// Prepare small UAV barriers to separate tracing/sampling from dispatch generation
	// (since afaik there's no easy way to make indirect dispatch arguments scale with shader threads/group otherwise)
	// All rendering shaders write to the Philox state buffer immediately before exiting
	D3D12_RESOURCE_BARRIER dispEditBarriers[2] = { AthruGPU::UAVBarrier(randState.resrc),
												   AthruGPU::UAVBarrier(randState.resrc) };

	// Prepare path-tracing commands
	ptCmdList->SetDescriptorHeaps(1, viewHeap.GetAddressOf());
	ptCmdList->SetComputeRootSignature(tracer.rootSig.Get()); // Tracing and sampling passes share the same descriptor layout
	ptCmdList->SetComputeRootDescriptorTable(0, baseRndrDescriptor);
	for (int i = 0; i < GraphicsStuff::MAX_NUM_BOUNCES; i += 1)
	{
		// Perform ray-tracing/marching
		ptCmdList->SetPipelineState(tracer.shadingState.Get());
		if (i > 0)
		{
			// Match tracing dispatches to remaining paths after the zeroth bounce
			ptCmdList->ExecuteIndirect(ptCmdSignature.Get(), 1, dispAxesArgs.resrc.Get(), 0, nullptr, NULL);
		}
		else
		{
			// Use a standard full-screen dispatch when counters are uninitialized
			ptCmdList->Dispatch(GraphicsStuff::TILING_AREA / 256, 1, 1);
		}

		// Generate sampling dispatch axes
		ptCmdList->ResourceBarrier(1, dispEditBarriers);
		ptCmdList->SetPipelineState(dispEditor.shadingState.Get());
		ptCmdList->Dispatch(1, 1, 1);

		// Update sampler counters
		if (i == 0) { ptCmdList->ResourceBarrier(1, dispBarriers); } // Consider issuing both elements and using split barriers here
		else { ptCmdList->ResourceBarrier(2, dispBarriers); }
		ptCmdList->CopyBufferRegion(dispAxesArgs.resrc.Get(), 0, rndrBuff.resrc.Get(), (u8Byte)(offsets[7]) + 12, 12);
		ptCmdList->ResourceBarrier(2, dispBarriers + 2);

		// Perform surface sampling & generate traceable dispatch axes
		ptCmdList->SetPipelineState(surfSampler.shadingState.Get());
		ptCmdList->ExecuteIndirect(ptCmdSignature.Get(), 1, dispAxesArgs.resrc.Get(), 0, nullptr, NULL);
		ptCmdList->ResourceBarrier(1, dispEditBarriers + 1);
		ptCmdList->SetPipelineState(dispEditor.shadingState.Get());
		ptCmdList->Dispatch(1, 1, 1);

		// Update ray-tracing/marching counters (tracing/marching occurs as a full-screen pass in the zeroth iteration)
		ptCmdList->ResourceBarrier(2, dispBarriers); // Consider issuing both elements and using split barriers here
		ptCmdList->CopyBufferRegion(dispAxesArgs.resrc.Get(), 0, rndrBuff.resrc.Get(), offsets[7], 12);
		ptCmdList->ResourceBarrier(2, dispBarriers + 2);
	}
	ptCmdList->Close();

	// Prepare static data for presentation barriers
	D3D12_RESOURCE_BARRIER copyBarriers[4];
	copyBarriers[0] = AthruGPU::TransitionBarrier(D3D12_RESOURCE_STATE_COPY_DEST,
												  nullptr,
												  D3D12_RESOURCE_STATE_PRESENT);
	copyBarriers[1] = AthruGPU::TransitionBarrier(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE,
												  displayTexLDR.resrc,
												  displayTexLDR.resrcState);
	copyBarriers[2] = AthruGPU::TransitionBarrier(D3D12_RESOURCE_STATE_PRESENT,
												  nullptr,
												  D3D12_RESOURCE_STATE_COPY_DEST);
	copyBarriers[3] = copyBarriers[1];
	copyBarriers[3].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
	copyBarriers[3].Transition.StateAfter = displayTexLDR.resrcState;

	// Prepare commands to post-process + present each buffer in the swap-chain
	Direct3D* d3d = AthruGPU::GPU::AccessD3D();
	for (u4Byte i = 0; i < AthruGPU::NUM_SWAPCHAIN_BUFFERS; i += 1)
	{
		// Create a command-list for the current back-buffer
		device->CreateCommandList(0x1,
								  D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
								  rnderAlloc.Get(),
								  post.shadingState.Get(),
								  __uuidof(postCmdLists[i]),
								  (void**)(&postCmdLists[i]));

		// Prepare post-processing commands
		postCmdLists[i]->SetDescriptorHeaps(1, viewHeap.GetAddressOf());
		// -- No pixel resource barrier here, sampled colors are staged per-path and copied onto the display texture during post-processing --
		postCmdLists[i]->SetComputeRootSignature(post.rootSig.Get());
		postCmdLists[i]->SetComputeRootDescriptorTable(0, baseRndrDescriptor);
		postCmdLists[i]->Dispatch(disp.x, disp.y, disp.z);

		// Prepare presentation commands
		////////////////////////////////

		// Copy the the display texture into the current back-buffer
		// Direct copies don't seem to work (can only access the zeroth back-buffer index?), should ask
		// CGSE about options here
		// Lots of stack-local logic here, might need to move those into persistent rendering state
		D3D12_TEXTURE_COPY_LOCATION texSrc;
		texSrc.pResource = displayTexLDR.resrc.Get();
		texSrc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		texSrc.SubresourceIndex = 0; // No mip-maps on the display texture
		D3D12_TEXTURE_COPY_LOCATION texDst;
		Microsoft::WRL::ComPtr<ID3D12Resource> backBufTx;
		d3d->GetBackBuf(backBufTx, i);
		texDst.pResource = backBufTx.Get();
		texDst.SubresourceIndex = 0; // No mip-maps on the back-buffer
		texDst.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		// Transition to copyable resources & perform copy to the back-buffer
		copyBarriers[0].Transition.pResource = backBufTx.Get();
		postCmdLists[i]->ResourceBarrier(2, copyBarriers);
		postCmdLists[i]->CopyTextureRegion(&texDst, 0, 0, 0, &texSrc, nullptr); // Perform copy to the back-buffer

		// Transition back to standard-use resources
		copyBarriers[2].Transition.pResource = backBufTx.Get();
		postCmdLists[i]->ResourceBarrier(2, copyBarriers + 2);
		postCmdLists[i]->Close(); // End generic graphics submissions
	}

	// Compose command-list sets for batched submission
	for (u4Byte i = 0; i < AthruGPU::NUM_SWAPCHAIN_BUFFERS; i += 1)
	{
		rnderCmdSets[i][0] = lensList;
		rnderCmdSets[i][1] = ptCmdList;
		rnderCmdSets[i][2] = postCmdLists[i];
	}
}

Renderer::~Renderer()
{
	// Wait for the graphics queue before freeing memory
	AthruGPU::GPU::AccessD3D()->WaitForQueue<D3D12_COMMAND_LIST_TYPE_DIRECT>();

	// Release shader data
	lensSampler.~ComputePass();
	tracer.~ComputePass();
	surfSampler.~ComputePass();
	post.~ComputePass();
}

void Renderer::Render(Direct3D* d3d)
{
	// Execute prepared commands
	rnderQueue->ExecuteCommandLists(3, (ID3D12CommandList**)rnderCmdSets[rnderFrameCtr % 3]);
	rnderFrameCtr += 1;
	d3d->WaitForQueue<D3D12_COMMAND_LIST_TYPE_DIRECT>();

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
