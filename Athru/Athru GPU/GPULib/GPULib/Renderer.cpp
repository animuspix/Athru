#include "GPUServiceCentre.h"
#include "Renderer.h"
#include "PixHistory.h"
#include "Camera.h"
#include "GPUMemory.h"
#include <functional>

Renderer::Renderer(HWND windowHandle,
				   const AthruGPU::GPUMemory& gpuMem,
				   const Microsoft::WRL::ComPtr<ID3D12Device>& device,
				   const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& rndrCmdQueue,
				   const Microsoft::WRL::ComPtr<ID3D12CommandList>& rndrCmdList,
				   const Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& rndrCmdAlloc) :
			tracers{ ComputePass(device,
								 windowHandle,
								 "LensSampler.cso",
								 AthruGPU::RESRC_CTX::RNDR_OR_GENERIC),
					 ComputePass(device,
					 			 windowHandle,
					 			 "RayMarch.cso",
								 AthruGPU::RESRC_CTX::RNDR_OR_GENERIC) },
			bouncePrep(device,
					   windowHandle,
					   "BouncePrep.cso",
					   AthruGPU::RESRC_CTX::RNDR_OR_GENERIC),
			samplers{ ComputePass(device,
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
			screenPainter(device,
						  windowHandle,
						  L"PresentationVerts.cso",
						  L"PresentationColors.cso"),
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
	// Construct the render-specific input buffer
	renderInputBuffer.InitBuf(device, gpuMem);

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
						  &(ctrBuf.resrc),
						  GraphicsStuff::TILING_AREA,
						  RNDR_CTR_OFFSET_GENERIC);

	// Create the intersection/surface buffer
	surfIsections.InitAppBuf(device,
							 gpuMem,
							 &(ctrBuf.resrc),
						     GraphicsStuff::TILING_AREA,
							 RNDR_CTR_OFFSET_GENERIC);

	// Create the diffuse intersection buffer
	diffuIsections.InitAppBuf(device,
							  gpuMem,
							  &(ctrBuf.resrc),
						      GraphicsStuff::TILING_AREA,
							  0);

	// Create the mirrorlike intersection buffer
	mirroIsections.InitAppBuf(device,
							  gpuMem,
							  &(ctrBuf.resrc),
						      GraphicsStuff::TILING_AREA,
							  12);

	// Create the refractive intersection buffer
	refraIsections.InitAppBuf(device,
							  gpuMem,
							  &(ctrBuf.resrc),
						      GraphicsStuff::TILING_AREA,
							  24);

	// Create the snowy intersection buffer
	snowwIsections.InitAppBuf(device,
							  gpuMem,
							  &(ctrBuf.resrc),
						      GraphicsStuff::TILING_AREA,
							  36);

	// Create the organic/sub-surface scattering intersection buffer
	ssurfIsections.InitAppBuf(device,
							  gpuMem,
							  &(ctrBuf.resrc),
						      GraphicsStuff::TILING_AREA,
							  48);

	// Create the furry intersection buffer
	furryIsections.InitAppBuf(device,
							  gpuMem,
							  &(ctrBuf.resrc),
						      GraphicsStuff::TILING_AREA,
							  60);

	// Construct the presentation-only constant buffer
	//DirectX::XMFLOAT4 displayInfo = DirectX::XMFLOAT4(GraphicsStuff::DISPLAY_WIDTH,
	//												  GraphicsStuff::DISPLAY_HEIGHT,
	//												  GraphicsStuff::DISPLAY_AREA,
	//												  GraphicsStuff::NUM_AA_SAMPLES);
	//displayInputBuffer = AthruGPU::AthruBuffer<DirectX::XMFLOAT4, AthruGPU::CBuffer>(device,
	//																				 (void*)&displayInfo);
}

Renderer::~Renderer(){}

void Renderer::Render(Camera* camera)
{
	// Dispatch lens sampler
	// [wait]
	// Dispatch path-tracing bundle in a tight loop
	// [wait]
	// Dispatch post-processing bundle & copy onto the back-buffer

	// Load PRNG states, generate camera directions + filter values,
	// initialize the intersection buffer
	const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& displayTexRW = camera->GetViewFinder()->GetDisplayTex().readWriteView;
	this->SampleLens(camera->GetTranslation(),
					 camera->GetViewMatrix(),
					 displayTexRW);
	// Perform path-tracing
	// One extra bounce to account for primary ray emission
	// All rays at each bounce are guaranteed to have the same depth, so we can perform path
	// cancellation here instead of in-shader for better performance
	for (int i = 0; i < GraphicsStuff::MAX_NUM_BOUNCES; i += 1) // Avoid infinite paths by clipping rays after
																// [MAX_NUM_BOUNCES]
	{
		// Scale counter buffer to 128 threads/group before tracing each bounce
		context->CSSetShader(dispatchScale256.shader.Get(),
							 nullptr, 0);
		context->Dispatch(1, 1, 1);

		// Propagate rays out to intersection
		this->Trace();

		// Perform bounce operations
		// Non-atmospheric/cloudy refractors (snow/fur/generic-subsurface/generic-refractors)
		// update ray position as well as direction (atmospheric + cloudy scattering is
		// computed in-line with ordinary ray-marching)
		this->Bounce(displayTexRW);
	}

	// Filter generated results + tonemap, then copy to the display texture
	this->Prepare();

	// Denoise, publish traced results to the display
	this->Present(camera->GetViewFinder());
}

void Renderer::SampleLens(DirectX::XMVECTOR& cameraPosition,
						  DirectX::XMMATRIX& viewMatrix,
						  const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& displayTexRW)
{
	// Pass the pre-processing shader onto the GPU
	context->CSSetShader(tracers[0].shader.Get(), nullptr, 0);

	// Pass all the path-tracing elements we expect to use onto the GPU as well
	// Path-tracing needs random ray directions, so pass a write-allowed view of our shader-friendly
	// random-number buffer onto the GPU over here
	const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& gpuRandView = AthruGPU::GPU::AccessGPURandView();
	context->CSSetUnorderedAccessViews(1, 1, gpuRandView.GetAddressOf(), 0);

	// This render-stage publishes to the screen, so we need to pass the
	// display texture along to the GPU
	context->CSSetUnorderedAccessViews(0, 1, displayTexRW.GetAddressOf(), 0);

	// Sampled directions are fed to the intersection shader through [traceables], so push that along
	// as well
	u4Byte initTraceCtr = 0;
	context->CSSetUnorderedAccessViews(3, 1, traceables.view().GetAddressOf(), &initTraceCtr);

	// Later we'll also need the material intersection buffer, the surface intersection buffer, and the
	// material counter-buffer; get all those onto the GPU here
	// (+ also push our AA integration buffer)
	context->CSSetUnorderedAccessViews(6, 1, surfIsections.view().GetAddressOf(), 0);
	context->CSSetUnorderedAccessViews(4, 1, ctrBuf.view().GetAddressOf(), 0);

	// Expose the local input buffer for writing
	D3D11_MAPPED_SUBRESOURCE shaderInput;
	HRESULT result = context->Map(renderInputBuffer.buf.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &shaderInput);
	assert(SUCCEEDED(result));

	// Get a pointer to the raw data in the shader input buffer
	RenderInput* shaderInputPtr;
	shaderInputPtr = (RenderInput*)shaderInput.pData;

	// Copy in the camera position + view matrix (also the inverse view matrix)
	shaderInputPtr->cameraPos = cameraPosition;
	shaderInputPtr->viewMat = viewMatrix;
	shaderInputPtr->iViewMat = DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(viewMatrix),
														  viewMatrix);

	// Update bounce information
	shaderInputPtr->bounceInfo = DirectX::XMFLOAT4(GraphicsStuff::MAX_NUM_BOUNCES,
												   GraphicsStuff::NUM_SUPPORTED_SURF_BXDFS,
												   GraphicsStuff::EPSILON_MAX, // Large epsilon distance to match with interplanetary distances in
													 						   // the test scene
												   0); // [w] is unused

	// Update [resInfo]
	shaderInputPtr->resInfo = DirectX::XMUINT4(GraphicsStuff::DISPLAY_WIDTH,
											   GraphicsStuff::DISPLAY_HEIGHT,
											   GraphicsStuff::NUM_AA_SAMPLES,
											   GraphicsStuff::DISPLAY_AREA);

	// Update [tilingInfo]
	shaderInputPtr->tilingInfo = DirectX::XMUINT4(GraphicsStuff::TILING_WIDTH,
												  GraphicsStuff::TILING_HEIGHT,
												  GraphicsStuff::TILING_AREA,
												  0);

	// Update [tileInfo]
	shaderInputPtr->tileInfo = DirectX::XMUINT4(GraphicsStuff::TILE_WIDTH,
												GraphicsStuff::TILE_HEIGHT,
												GraphicsStuff::TILE_AREA,
												0);

	// Break the write-allowed connection to the shader input buffer
	context->Unmap(renderInputBuffer.buf.Get(), 0);

	// Pass the data in the local input buffer over to the GPU
	context->CSSetConstantBuffers(1, 1, renderInputBuffer.buf.GetAddressOf());

	// Dispatch the lens sampler
	DirectX::XMUINT3 disp = AthruGPU::tiledDispatchAxes(16);
	context->Dispatch(disp.x,
					  disp.y,
					  disp.z);
}

void Renderer::Trace()
{
	// Optional counter-buffer debug code
	//#define CTR_DBG
	#ifdef CTR_DBG
		AthruGPU::AthruBuffer<u4Byte, AthruGPU::StgBuffer> buff = AthruGPU::AthruBuffer<u4Byte, AthruGPU::StgBuffer>(AthruGPU::GPU::AccessD3D()->GetDevice(),
																													 nullptr,
																													 30,
																													 DXGI_FORMAT_R32_UINT);
		context->CopyResource(buff.buf.Get(), ctrBuf.buf.Get());
		D3D11_MAPPED_SUBRESOURCE ctr;
		context->Map(buff.buf.Get(), 0, D3D11_MAP_READ, 0, &ctr);
		u4Byte* ctrRef = (u4Byte*)(ctr.pData);
		context->Unmap(buff.buf.Get(), 0);
	#endif
	// Optional length-check for the traceables buffer
	//#define TRACEABLES_DBG
	#ifdef TRACEABLES_DBG
		u4Byte traceableLen = AthruGPU::appConsumeCount<LiBounce>(context,
																  AthruGPU::GPU::AccessD3D()->GetDevice(),
																  traceables);
	#endif
	context->CSSetShader(tracers[1].shader.Get(), nullptr, 0);
	context->DispatchIndirect(ctrBuf.buf.Get(),
							  RNDR_CTR_OFFSET_GENERIC);
	// Scale output dispatch arguments to match the threads/group in [BouncePrep]
	context->CSSetShader(dispatchScale256.shader.Get(),
						 nullptr, 0);
	context->Dispatch(1, 1, 1);
}

void Renderer::Bounce(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& displayTex)
{
	// Pass material synthesis/sorting state to the GPU
	context->CSSetShader(bouncePrep.shader.Get(), nullptr, 0);
	context->CSSetUnorderedAccessViews(0, 1, diffuIsections.view().GetAddressOf(), 0);
	context->CSSetUnorderedAccessViews(1, 1, mirroIsections.view().GetAddressOf(), 0);
	context->CSSetUnorderedAccessViews(2, 1, refraIsections.view().GetAddressOf(), 0);
	context->CSSetUnorderedAccessViews(3, 1, snowwIsections.view().GetAddressOf(), 0);
	context->CSSetUnorderedAccessViews(5, 1, ssurfIsections.view().GetAddressOf(), 0);
	context->CSSetUnorderedAccessViews(7, 1, furryIsections.view().GetAddressOf(), 0);
	context->DispatchIndirect(ctrBuf.buf.Get(),
							  RNDR_CTR_OFFSET_GENERIC); // Perform material synthesis/sorting

	//#define ISECTIONS_DBG
	#ifdef ISECTIONS_DBG
		u4Byte traceableLen = AthruGPU::appConsumeCount<LiBounce>(context,
																  AthruGPU::GPU::AccessD3D()->GetDevice(),
																  diffuIsections);
	#endif

	// Scale output dispatch arguments to match threads/group for the sampling
	// shaders
	context->CSSetShader(dispatchScale256.shader.Get(),
						 nullptr, 0);
	context->Dispatch(1, 1, 1);

	// Regenerate resource bindings displaced during material synthesis/sorting
	context->CSSetUnorderedAccessViews(3, 1, traceables.view().GetAddressOf(), nullptr);
	context->CSSetUnorderedAccessViews(1, 1, AthruGPU::GPU::AccessGPURandView().GetAddressOf(), nullptr);
	context->CSSetUnorderedAccessViews(0, 1, displayTex.GetAddressOf(), nullptr);

	// Local sampling function
	auto MatSampler =
	[this](const u4Byte& matNdx,
		   const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& matBuffer)
	{
		context->CSSetShader(samplers[matNdx].shader.Get(), nullptr, 0);
		context->CSSetUnorderedAccessViews(6, 1, matBuffer.GetAddressOf(), nullptr);
		context->DispatchIndirect(ctrBuf.buf.Get(),
								  AthruGPU::DISPATCH_ARGS_SIZE * matNdx);
		ID3D11UnorderedAccessView* nullUAV = nullptr;
		context->CSSetUnorderedAccessViews(6, 1, &nullUAV, 0);
	};
	MatSampler((u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::DIFFU, diffuIsections.view());
	//MatSampler((u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::MIRRO, mirroIsections.view());
	//MatSampler((u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::REFRA, refraIsections.view());
	//MatSampler((u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::SNOWW, snowwIsections.view());
	//MatSampler((u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::SSURF, ssurfIsections.view());
	//MatSampler((u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::FURRY, furryIsections.view());

	// Optional counter-buffer debug code
	//#define CTR_DBG
	#ifdef CTR_DBG
		AthruGPU::AthruBuffer<u4Byte, AthruGPU::StgBuffer> buff = AthruGPU::AthruBuffer<u4Byte, AthruGPU::StgBuffer>(AthruGPU::GPU::AccessD3D()->GetDevice(),
																													 nullptr,
																													 30,
																													 DXGI_FORMAT_R32_UINT);
		context->CopyResource(buff.buf.Get(), ctrBuf.buf.Get());
		D3D11_MAPPED_SUBRESOURCE ctr;
		context->Map(buff.buf.Get(), 0, D3D11_MAP_READ, 0, &ctr);
		u4Byte* ctrRef = (u4Byte*)(ctr.pData);
		context->Unmap(buff.buf.Get(), 0);
	#endif
}

void Renderer::Prepare()
{
	context->CSSetShader(post.shader.Get(), nullptr, 0);
	context->CSSetUnorderedAccessViews(2, 1, aaBuffer.view().GetAddressOf(), 0);
	DirectX::XMUINT3 disp = AthruGPU::tiledDispatchAxes(16);
	context->Dispatch(disp.x,
					  disp.y,
					  disp.z);

	// Resources can't be exposed through unordered-access-views (read/write allowed) and shader-
	// resource-views (read-only) simultaneously, so un-bind the local unordered-access-view
	// of the display texture here
	ID3D11UnorderedAccessView* nullUAV = nullptr;
	context->CSSetUnorderedAccessViews(0, 1, &nullUAV, 0);
}

void Renderer::Present(ViewFinder* viewFinder)
{
	viewFinder->GetViewGeo().PassToGPU(context);
	screenPainter.Render(context,
						 displayInputBuffer.buf,
					     viewFinder->GetDisplayTex().readOnlyView);
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