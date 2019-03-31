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
			bdxfs{ ComputePass(device,
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

	// Create the counter buffer
	// Likely more efficient to define this as mostly-GPU and initialize on the zeroth frame than to upload
	// single-use initial data on startup
	ctrBuf.InitBuf(device,
				   gpuMem,
				   30,
				   DXGI_FORMAT::DXGI_FORMAT_R32_UINT);

	// Create the tracing append/consume buffer
	traceables.InitAppBuf(device,
						  gpuMem,
						  ctrBuf.resrc,
						  GraphicsStuff::TILING_AREA,
						  RNDR_CTR_OFFSET_GENERIC);

	// Create the intersection/surface buffer
	surfIsections.InitAppBuf(device,
							 gpuMem,
							 ctrBuf.resrc,
						     GraphicsStuff::TILING_AREA,
							 RNDR_CTR_OFFSET_GENERIC);

	// Create the diffuse intersection buffer
	diffuIsections.InitAppBuf(device,
							  gpuMem,
							  ctrBuf.resrc,
						      GraphicsStuff::TILING_AREA,
							  0);

	// Create the mirrorlike intersection buffer
	mirroIsections.InitAppBuf(device,
							  gpuMem,
							  ctrBuf.resrc,
						      GraphicsStuff::TILING_AREA,
							  12);

	// Create the refractive intersection buffer
	refraIsections.InitAppBuf(device,
							  gpuMem,
							  ctrBuf.resrc,
						      GraphicsStuff::TILING_AREA,
							  24);

	// Create the snowy intersection buffer
	snowwIsections.InitAppBuf(device,
							  gpuMem,
							  ctrBuf.resrc,
						      GraphicsStuff::TILING_AREA,
							  36);

	// Create the organic/sub-surface scattering intersection buffer
	ssurfIsections.InitAppBuf(device,
							  gpuMem,
							  ctrBuf.resrc,
						      GraphicsStuff::TILING_AREA,
							  48);

	// Create the furry intersection buffer
	furryIsections.InitAppBuf(device,
							  gpuMem,
							  ctrBuf.resrc,
						      GraphicsStuff::TILING_AREA,
							  60);

	// Create the output texture
	displayTex.InitRWTex(device, gpuMem,
						 GraphicsStuff::DISPLAY_WIDTH,
						 GraphicsStuff::DISPLAY_HEIGHT);

    // Prepare rendering commands
    /////////////////////////////

    // Prepare driver settings for rendering
    rndrCmdAlloc->Reset();
    rndrCmdList->SetDescriptorHeaps(1, gpuMem.GetShaderViewMem().GetAddressOf());

	// Most bindings should have already occurred during resource + pipeline-state setup
    // Prepare lens-sampling commands
	DirectX::XMUINT3 disp = AthruGPU::tiledDispatchAxes(16);
    rndrCmdList->SetPipelineState(lensSampler.shadingState.Get());
	rndrCmdList->Dispatch(disp.x, disp.y, disp.z);

    // Prepare path-tracing commands
    for (int i = 0; i < GraphicsStuff::MAX_NUM_BOUNCES; i += 1)
    {
        // Data readback should stall out the GPU as much as UAV barriers
        // Update CPU counter data before tracing
        rndrCmdList->SetPipelineState(tracer.shadingState.Get());
        //rndrCmdList->Dispatch(); // Dispatch arguments driven from readback counter information
        // Update CPU counter data before bounce preparation
        rndrCmdList->SetPipelineState(bouncePrep.shadingState.Get());
        //rndrCmdList->Dispatch(); // Dispatch arguments driven from readback counter information
        // Update CPU counter data before sampling materials
        rndrCmdList->SetPipelineState(bdxfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::DIFFU].shadingState.Get());
        //rndrCmdList->Dispatch(); // Dispatch arguments driven from readback counter information
        rndrCmdList->SetPipelineState(bdxfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::MIRRO].shadingState.Get());
        //rndrCmdList->Dispatch(); // Dispatch arguments driven from readback counter information
        rndrCmdList->SetPipelineState(bdxfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::REFRA].shadingState.Get());
        //rndrCmdList->Dispatch(); // Dispatch arguments driven from readback counter information
        rndrCmdList->SetPipelineState(bdxfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::SNOWW].shadingState.Get());
        //rndrCmdList->Dispatch(); // Dispatch arguments driven from readback counter information
        rndrCmdList->SetPipelineState(bdxfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::SSURF].shadingState.Get());
        //rndrCmdList->Dispatch(); // Dispatch arguments driven from readback counter information
        rndrCmdList->SetPipelineState(bdxfs[(u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::FURRY].shadingState.Get());
        //rndrCmdList->Dispatch(); // Dispatch arguments driven from readback counter information
    }

    // Prepare post-processing commands
    D3D12_RESOURCE_BARRIER barrier = AthruGPU::UAVBarrier(traceables.resrc);
	rndrCmdList->ResourceBarrier(1, &barrier);
    rndrCmdList->SetPipelineState(post.shadingState.Get());
	rndrCmdList->Dispatch(disp.x, disp.y, disp.z);

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
    rndrCmdList->ResourceBarrier(2, copyBarriers);
    rndrCmdList->CopyTextureRegion(&texDst, 0, 0, 0, &texSrc, nullptr); // Perform copy to the back-buffer

    // Transition back to standard-use resources
    copyBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
    copyBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT;
    copyBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE;
    copyBarriers[1].Transition.StateAfter = displayTex.resrcState;
    rndrCmdList->ResourceBarrier(2, copyBarriers);
    rndrCmdList->Close(); // End graphics submissions for the current frame
    // -- Might need an extra sync here, unsure --
}

Renderer::~Renderer(){}

void Renderer::Render(Direct3D* d3d)
{
    // Execute rendering commands
    d3d->GetGraphicsQueue()->ExecuteCommandLists(1, (ID3D12CommandList**)renderCmdList.GetAddressOf()); // Execute graphics commands

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