#include "GPUServiceCentre.h"
#include "Renderer.h"
#include "PixHistory.h"
#include "Camera.h"
#include <functional>

Renderer::Renderer(HWND windowHandle,
				   const Microsoft::WRL::ComPtr<ID3D11Device>& device,
				   const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& d3dContext) :
			tracers{ ComputeShader(device,
								   windowHandle,
								   L"LensSampler.cso"),
					 ComputeShader(device,
					 			   windowHandle,
					 			   L"RayMarch.cso") },
			bouncePrep(device,
					   windowHandle,
					   L"BouncePrep.cso"),
			samplers{ ComputeShader(device,
									windowHandle,
									L"DiffuSampler.cso"),
					  ComputeShader(device,
									windowHandle,
									L"MirroSampler.cso"),
					  ComputeShader(device,
									windowHandle,
									L"RefraSampler.cso"),
					  ComputeShader(device,
									windowHandle,
									L"SnowwSampler.cso"),
					  ComputeShader(device,
									windowHandle,
									L"SsurfSampler.cso"),
					  ComputeShader(device,
									windowHandle,
									L"FurrySampler.cso") },
			post(device,
				 windowHandle,
				 L"RasterPrep.cso"),
			screenPainter(device,
						  windowHandle,
						  L"PresentationVerts.cso",
						  L"PresentationColors.cso"),
			dispatchScale128(device,
							 windowHandle,
							 L"dispScale128.cso"),
			dispatchScale256(device,
							 windowHandle,
							 L"dispScale256.cso"),
			dispatchScale512(device,
							 windowHandle,
							 L"dispScale512.cso"),
			context(d3dContext)
{
	// Construct the render-specific input buffer
	renderInputBuffer = AthruBuffer<RenderInput, GPGPUStuff::CBuffer>(device,
																	  nullptr);

	// Build the buffer we'll be using to store
	// image samples for temporal smoothing/anti-aliasing
	aaBuffer = AthruBuffer<PixHistory, GPGPUStuff::GPURWBuffer>(device,
																nullptr,
																GraphicsStuff::DISPLAY_AREA);
	// Create the tracing append/consume buffer
	traceables = AthruBuffer<LiBounce, GPGPUStuff::AppBuffer>(device,
															  nullptr,
															  GraphicsStuff::DISPLAY_AREA);

	// Create the intersection/surface buffer
	surfIsections = AthruBuffer<LiBounce, GPGPUStuff::AppBuffer>(device,
																 nullptr,
																 GraphicsStuff::DISPLAY_AREA);

	// Create the diffuse intersection buffer
	diffuIsections = AthruBuffer<LiBounce, GPGPUStuff::AppBuffer>(device,
																  nullptr,
																  GraphicsStuff::DISPLAY_AREA);
	// Create the mirrorlike intersection buffer
	mirroIsections = AthruBuffer<LiBounce, GPGPUStuff::AppBuffer>(device,
																  nullptr,
																  GraphicsStuff::DISPLAY_AREA);
	// Create the refractive intersection buffer
	refraIsections = AthruBuffer<LiBounce, GPGPUStuff::AppBuffer>(device,
																  nullptr,
																  GraphicsStuff::DISPLAY_AREA);
	// Create the snowy intersection buffer
	snowwIsections = AthruBuffer<LiBounce, GPGPUStuff::AppBuffer>(device,
															      nullptr,
															      GraphicsStuff::DISPLAY_AREA);

	// Create the organic/sub-surface scattering intersection buffer
	ssurfIsections = AthruBuffer<LiBounce, GPGPUStuff::AppBuffer>(device,
																  nullptr,
																  GraphicsStuff::DISPLAY_AREA);

	// Create the furry intersection buffer
	furryIsections = AthruBuffer<LiBounce, GPGPUStuff::AppBuffer>(device,
																  nullptr,
																  GraphicsStuff::DISPLAY_AREA);

	// Create the material counter + its matching staging resource
	u4Byte ctrs[30] = { 0, 0, 0, // Initially zero per-material dispatch sizes
						0, 0, 0,
						0, 0, 0,
						0, 0, 0,
						0, 0, 0,
						0, 0, 0,
						0, 0, 0, // Initially zero generic dispatch sizes
						1, 0, 0, // One thread/group assumed by default, initially zero light bounces,
						0, 0, 0, // initially no generic elements, initially no materials
						0, 0, 0 };
	ctrBuf = AthruBuffer<u4Byte, GPGPUStuff::CtrBuffer>(device,
														&ctrs,
														30,
														DXGI_FORMAT::DXGI_FORMAT_R32_UINT);

	// Construct the presentation-only constant buffer
	DirectX::XMFLOAT4 displayInfo = DirectX::XMFLOAT4(GraphicsStuff::DISPLAY_WIDTH,
													  GraphicsStuff::DISPLAY_HEIGHT,
													  GraphicsStuff::DISPLAY_AREA,
													  GraphicsStuff::NUM_AA_SAMPLES);
	displayInputBuffer = AthruBuffer<DirectX::XMFLOAT4, GPGPUStuff::CBuffer>(device,
																			 (void*)&displayInfo);
}

Renderer::~Renderer(){}

void Renderer::Render(Camera* camera)
{
	// Load PRNG states, generate camera directions + filter values,
	// initialize the intersection buffer
	const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& displayTexRW = camera->GetViewFinder()->GetDisplayTex().readWriteView;
	this->SampleLens(camera->GetTranslation(),
					 camera->GetViewMatrix(),
					 displayTexRW);
	// Perform path-tracing
	// One extra bounce to account for primary ray emission
	// All rays at each bounce are guaranteed to have the same depth, so we can perform Russian Roulette
	// cancellation here instead of in-shader for better performance
	for (int i = 0; i < GraphicsStuff::MAX_NUM_BOUNCES; i += 1) // Avoid infinite paths by clipping rays after
																// [MAX_NUM_BOUNCES]
	{
		// Scale counter buffer to 128 threads/group before tracing each bounce
		context->CSSetShader(dispatchScale128.shader.Get(),
							 nullptr, 0);
		context->Dispatch(1, 1, 1);

		// Propagate rays out to intersection
		this->Trace();
		break;

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
	context->CSSetUnorderedAccessViews(3, 1, traceables.view().GetAddressOf(), 0);

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

	// Break the write-allowed connection to the shader input buffer
	context->Unmap(renderInputBuffer.buf.Get(), 0);

	// Pass the data in the local input buffer over to the GPU
	context->CSSetConstantBuffers(1, 1, renderInputBuffer.buf.GetAddressOf());

	// Dispatch the pre-processing shader
	context->Dispatch(GraphicsStuff::DISPLAY_WIDTH / GraphicsStuff::GROUP_WIDTH_PATH_REDUCTION,
					  GraphicsStuff::DISPLAY_HEIGHT / GraphicsStuff::GROUP_WIDTH_PATH_REDUCTION,
					  1);
}

void Renderer::Trace()
{
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
	std::function<void(const u4Byte&,
					   const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>&)> MatSampler =
	[this](const u4Byte& matNdx,
		   const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& matBuffer)
	{
		context->CSSetShader(samplers[matNdx].shader.Get(), nullptr, 0);
		context->CSSetUnorderedAccessViews(6, 1, matBuffer.GetAddressOf(), nullptr);
		context->DispatchIndirect(ctrBuf.buf.Get(),
								  GPGPUStuff::DISPATCH_ARGS_SIZE * matNdx);
		ID3D11UnorderedAccessView* nullUAV = nullptr;
		context->CSSetUnorderedAccessViews(6, 1, &nullUAV, 0);
	};
	MatSampler((u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::DIFFU, diffuIsections.view());
	//MatSampler((u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::MIRRO, mirroIsections.view());
	//MatSampler((u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::REFRA, refraIsections.view());
	//MatSampler((u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::SNOWW, snowwIsections.view());
	//MatSampler((u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::SSURF, ssurfIsections.view());
	//MatSampler((u4Byte)GraphicsStuff::SUPPORTED_SURF_BXDDFS::FURRY, furryIsections.view());
}

void Renderer::Prepare()
{
	context->CSSetShader(post.shader.Get(), nullptr, 0);
	context->CSSetUnorderedAccessViews(2, 1, aaBuffer.view().GetAddressOf(), 0);
	context->Dispatch(GraphicsStuff::DISPLAY_WIDTH / GraphicsStuff::GROUP_WIDTH_PATH_REDUCTION,
					  GraphicsStuff::DISPLAY_HEIGHT / GraphicsStuff::GROUP_WIDTH_PATH_REDUCTION,
					  1);

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