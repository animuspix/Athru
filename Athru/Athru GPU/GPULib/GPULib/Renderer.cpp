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
			samplers{ ComputePass(device,
								 windowHandle,
								 "LensSampler.cso",
								 AthruGPU::RESRC_CTX::RNDR_OR_GENERIC),
					  ComputePass(device,
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
	// Build the buffer we'll be using to store
	// image samples for temporal smoothing/anti-aliasing
	aaBuffer.InitBuf(device,
					 gpuMem,
					 GraphicsStuff::DISPLAY_AREA);

	// Create the counter buffer
	// Likely more efficient to define this as GPU-only and initialize on the zeroth frame than to upload
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

	// Construct the lens-sampling allocator
	assert(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE,
													__uuidof(ID3D12CommandAllocator),
													(void**)lensCmds.GetAddressOf())));

	// Construct + record the lens-sampling command bundle
	assert(SUCCEEDED(device->CreateCommandList(0x1,
											   D3D12_COMMAND_LIST_TYPE_BUNDLE,
											   lensCmds.Get(),
											   samplers[0].shadingState.Get(),
											   __uuidof(ID3D12GraphicsCommandList),
											   (void**)sampleLens.GetAddressOf())));

	// Most bindings occur automatically during pipeline state setup, or are performed through the
	// generic rendering command list
	assert(SUCCEEDED(lensCmds->Reset()));
	DirectX::XMUINT3 disp = AthruGPU::tiledDispatchAxes(16);
	sampleLens->Dispatch(disp.x, disp.y, disp.z);
	assert(SUCCEEDED(sampleLens->Close()));

	// Construct the path-tracing iteration allocator
	assert(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE,
													__uuidof(ID3D12CommandAllocator),
													(void**)ptStepCmds.GetAddressOf())));

	// Construct + record the path-tracing iteration bundle
	assert(SUCCEEDED(device->CreateCommandList(0x1,
											   D3D12_COMMAND_LIST_TYPE_BUNDLE,
											   ptStepCmds.Get(),
											   tracer.shadingState.Get(),
											   __uuidof(ID3D12GraphicsCommandList),
											   (void**)ptStep.GetAddressOf())));
	assert(SUCCEEDED(ptStepCmds->Reset()));
	D3D12_RESOURCE_BARRIER barrier = AthruGPU::UAVBarrier(traceables.resrc);
	ptStep->ResourceBarrier(1, &barrier); // Likely to need explicit UAV barriers before each iteration
	// Trace/bounce-prep/bounce calls all depend on [ExecuteIndirect], need to read/watch more before implementing those here
	assert(SUCCEEDED(ptStep->Close()));

	// Construct the post-processing allocator
	assert(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE,
													__uuidof(ID3D12CommandAllocator),
													(void**)postProcCmds.GetAddressOf())));

	// Construct + record the post-processing bundle
	assert(SUCCEEDED(device->CreateCommandList(0x1,
											   D3D12_COMMAND_LIST_TYPE_BUNDLE,
											   postProcCmds.Get(),
											   post.shadingState.Get(),
											   __uuidof(ID3D12GraphicsCommandList),
											   (void**)postProc.GetAddressOf())));
	assert(SUCCEEDED(postProcCmds->Reset()));
	postProc->ResourceBarrier(1, &barrier);
	postProc->Dispatch(disp.x, disp.y, disp.z);
	assert(SUCCEEDED(postProc->Close()));
}

Renderer::~Renderer(){}

void Renderer::Render(Direct3D* d3d)
{
	// Load PRNG states, generate camera directions + filter values,
	// initialize the intersection buffer
	renderCmdList->ExecuteBundle(sampleLens.Get());

	// Perform path-tracing
	// One extra bounce to account for primary ray emission
	// All rays at each bounce are guaranteed to have the same depth, so we can perform path
	// cancellation here instead of in-shader for better performance
	for (int i = 0; i < GraphicsStuff::MAX_NUM_BOUNCES; i += 1) // Avoid infinite paths by clipping rays after
																// [MAX_NUM_BOUNCES]
	{
		// Implicit UAV barrier here
		renderCmdList->ExecuteBundle(ptStep.Get()); // Execute per-iteration path-tracing commands
	}

	// Filter generated results + tonemap + denoise, then copy to the display texture
	// Implicit UAV barrier here
	renderCmdList->ExecuteBundle(postProc.Get());

	// Publish traced results to the display
	// No UAV barrier, because presentation includes a transition barrier that should cause a similar wait
	d3d->Present(displayTex.resrc,
			     displayTex.resrcState);
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