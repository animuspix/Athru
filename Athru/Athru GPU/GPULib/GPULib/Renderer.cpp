#include "UtilityServiceCentre.h"
#include "GPUServiceCentre.h"
#include "Renderer.h"
#include "PixHistory.h"
#include "Camera.h"

Renderer::Renderer(HWND windowHandle,
				   const Microsoft::WRL::ComPtr<ID3D11Device>& device,
				   const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& d3dContext) :
			tracers{ ComputeShader(device,
								   windowHandle,
								   L"PathReduce.cso"),
					 ComputeShader(device,
					 			   windowHandle,
					 			   L"SceneVis.cso") },
			screenPainter(device,
						  windowHandle,
						  L"PresentationVerts.cso",
						  L"PresentationColors.cso"),
			context(d3dContext)
{
	// Construct the input buffer we'll be using
	// for shader i/o operations through [this]
	renderInputBuffer = AthruBuffer<InputStuffs, GPGPUStuff::CBuffer>(device,
																	  nullptr);

	// Build the buffer we'll be using to store
	// image samples for temporal
	// smoothing/anti-aliasing
	aaBuffer = AthruBuffer<PixHistory, GPGPUStuff::GPURWBuffer>(device,
																nullptr,
																GraphicsStuff::DISPLAY_AREA);
	// Create the intersection buffer
	// [traceables] are [float2x3]'s in shader code, so no reason to use
	// a fancy globally-visible discrete type when a small temporary
	// wrapper around an array of [XMFLOAT3]s will work too
	traceables = AthruBuffer<float2x3, GPGPUStuff::AppBuffer>(device,
															  nullptr,
															  GraphicsStuff::DISPLAY_AREA);

	// Create the buffer we'll use to capture [traceables]' hidden counter
	// value
	numTraceables = AthruBuffer<fourByteUnsigned, GPGPUStuff::StgBuffer>(device,
																	     nullptr,
																	     GraphicsStuff::DISPLAY_AREA);
	// Create the intersection counter buffer
	maxTraceablesCtr = AthruBuffer<fourByteSigned, GPGPUStuff::GPURWBuffer>(device,
																			nullptr,
																			1,
																			DXGI_FORMAT::DXGI_FORMAT_R32_SINT);
	// Create the staging buffer associated with [maxTraceables]
	maxTraceablesCtrCPU = AthruBuffer<fourByteSigned, GPGPUStuff::StgBuffer>(device,
																			 nullptr,
																			 1,
																			 DXGI_FORMAT::DXGI_FORMAT_R32_SINT);

	// Construct the presentation-only constant buffer
	DirectX::XMVECTOR displayInfo[1] = { _mm_set_ps(GraphicsStuff::DISPLAY_WIDTH,
													GraphicsStuff::DISPLAY_HEIGHT,
													GraphicsStuff::DISPLAY_AREA,
													GraphicsStuff::NUM_AA_SAMPLES) };
	displayInputBuffer = AthruBuffer<DisplayInfo, GPGPUStuff::CBuffer>(device,
																	   (void*)&displayInfo);

	// Initially zero [prevNumTraceables]
	prevNumTraceables = 0;
}

Renderer::~Renderer(){}

void Renderer::Render(Camera* camera)
{
	// Load PRNG states, generate camera directions + filter values,
	// initialize the intersection buffer
	this->PreProcess(camera->GetTranslation(),
					 camera->GetViewMatrix(),
					 camera->GetViewFinder()->GetDisplayTex().asWritableShaderResource);

	// Perform path-tracing
	// One extra bounce to account for primary ray emission
	fourByteUnsigned bounceCtr = 0;
	while (pathCount() > 0
		   && bounceCtr < GraphicsStuff::MAX_NUM_BOUNCES) // Only propagate rays if at least one path is still intersecting
														  // geometry, and enforce a hard limit of MAX_NUM_BOUNCES bounces/ray
														  // (since we still have to store bounces somewhere between ray
														  // propagation and shading)
	{
		// Propagate rays out to intersection, consume from the intersection buffer,
		// write to the shading buffer, increment material counters, increment scattering
		// counters
		this->Trace(camera->GetTranslation(),
					camera->GetViewMatrix());

		// Compute materials at intersection, perform next-event-estimation, apply russian
		// roulette ray cancellation (with bias for "dense" scenes with many deep paths),
		// decrement scattering counters as appropriate
		// Should additionally bias Russian Roulette by path type; incrementally greater
		// chance for cancellation after 7 bounces for every diffuse/specular material in
		// the path (90% chance for totally diffuse/specular paths)
		this->BounceProcessing();
 
		// Sample ray directions for the next bounce on each path, append to the intersection
		// buffer
		// Non-atmospheric/cloudy refractors (snow/fur/generic-subsurface/generic-refractors) 
		// update ray position as well as direction (atmospheric + cloudy scattering is 
		// computed in-line with ordinary ray-marching)
		this->SampleDiffu();
		this->SampleMirro();
		this->SampleRefra();
		this->SampleSnoww();
		this->SampleSSurf();
		this->SampleFurry();

		// Increment the bounce counter
		bounceCtr += 1;
	}

	// Compute shading at each bounce
	this->ShadeDiffu();
	this->ShadeMirro();
	this->ShadeRefra();
	this->ShadeSnoww();
	this->ShadeSSurf();
	this->ShadeFurry();

	// Integrate, publish traced results to the display
	this->Present(camera->GetViewFinder());

	// Ask the GPU to start processing committed rendering work
	AthruGPU::GPUServiceCentre::AccessD3D()->Output();
}

void Renderer::PreProcess(DirectX::XMVECTOR& cameraPosition,
						  DirectX::XMMATRIX& viewMatrix,
						  Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& displayTexWritable)
{
	// Pass the pre-processing shader onto the GPU
	context->CSSetShader(tracers[0].shader.Get(), nullptr, 0);

	// Pass all the path-tracing elements we expect to use onto the GPU as well
	// Path-tracing needs random ray directions, so pass a write-allowed view of our shader-friendly
	// random-number buffer onto the GPU over here
	const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& gpuRandView = AthruGPU::GPUServiceCentre::AccessGPURandView();
	context->CSSetUnorderedAccessViews(1, 1, gpuRandView.GetAddressOf(), 0);

	// This render-stage publishes to the screen, so we need to pass the
	// display texture along to the GPU
	context->CSSetUnorderedAccessViews(0, 1, displayTexWritable.GetAddressOf(), 0);

	// ...and it also exposes selected pixels to the path-tracer via [traceables], so we need to
	// pass that to the GPU as well
	// Also pass the max-traceables buffer along to the GPU (needed for SWR)
	context->CSSetUnorderedAccessViews(3, 1, traceables.view().GetAddressOf(), 0);
	context->CSSetUnorderedAccessViews(4, 1, maxTraceablesCtr.view().GetAddressOf(), 0);

	// We want to perform stratified anti-aliasing (basically tracing pseudo-random
	// subpixel rays each frame) and integrate the results over time, so push
	// the anti-aliasing buffer onto the GPU as well
	context->CSSetUnorderedAccessViews(2, 1, aaBuffer.view().GetAddressOf(), 0);

	// Expose the local input buffer for writing
	D3D11_MAPPED_SUBRESOURCE shaderInput;
	HRESULT result = context->Map(renderInputBuffer.buf.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &shaderInput);
	assert(SUCCEEDED(result));

	// Get a pointer to the raw data in the shader input buffer
	InputStuffs* shaderInputPtr;
	shaderInputPtr = (InputStuffs*)shaderInput.pData;

	// Copy in the camera position + view matrix (also the inverse view matrix)
	shaderInputPtr->cameraPos = cameraPosition;
	shaderInputPtr->viewMat = viewMatrix;
	shaderInputPtr->iViewMat = DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(viewMatrix),
														viewMatrix);

	// Copy in time/dispatch information for the current frame
	float dt = TimeStuff::deltaTime();
	std::chrono::nanoseconds currTimeNanoSecs = std::chrono::steady_clock::now().time_since_epoch();
	fourByteUnsigned tSecs = std::chrono::duration_cast<std::chrono::duration<fourByteUnsigned>>(currTimeNanoSecs).count();
	shaderInputPtr->timeDispInfo = DirectX::XMFLOAT4(dt, // Change-in-time in seconds
													 (float)tSecs, // Current time in seconds
													 (float)TimeStuff::frameCtr, // Current frame counter
													 GraphicsStuff::DISPLAY_AREA / GraphicsStuff::GROUP_AREA_PATH_REDUCTION); // Total group count (preprocessing)

	// Update the GPU-side version of [prevNumTraceables]
	shaderInputPtr->numTraceables = DirectX::XMFLOAT4(0.0f, // Current traceable-element count is undefined until after pre-processing
													 (float)prevNumTraceables,
													 0.0f, 0.0f);

	// Update the GPU-side version of [resInfo]
	shaderInputPtr->resInfo = DirectX::XMUINT4(GraphicsStuff::DISPLAY_WIDTH,
											   GraphicsStuff::DISPLAY_HEIGHT,
											   GraphicsStuff::NUM_AA_SAMPLES,
											   GraphicsStuff::DISPLAY_AREA);

	// Break the write-allowed connection to the shader input buffer
	context->Unmap(renderInputBuffer.buf.Get(), 0);

	// Pass the data in the local input buffer over to the GPU
	context->CSSetConstantBuffers(0, 1, renderInputBuffer.buf.GetAddressOf());

	// Dispatch the pre-processing shader
	context->Dispatch(GraphicsStuff::DISPLAY_WIDTH / GraphicsStuff::GROUP_WIDTH_PATH_REDUCTION,
					  GraphicsStuff::DISPLAY_HEIGHT / GraphicsStuff::GROUP_WIDTH_PATH_REDUCTION,
					  1);
}

void Renderer::Trace(DirectX::XMVECTOR& cameraPosition,
					 DirectX::XMMATRIX& viewMatrix)
{
	// Most of the shader inputs we expect to use were set during pre-processing, so no reason to
	// set them again over here
	context->CSSetShader(tracers[1].shader.Get(), nullptr, 0);

	// ...but we will have (usually) [Append()]'d some data to [traceables] during pre-processing,
	// so read the length of that back into a local value here
	D3D11_MAPPED_SUBRESOURCE traceableCount;
	context->CopyStructureCount(numTraceables.buf.Get(), 0, traceables.view().Get());
	context->Map(numTraceables.buf.Get(), 0, D3D11_MAP_READ, 0, &traceableCount);
	fourByteUnsigned traceableCounter = *(fourByteUnsigned*)(traceableCount.pData);
	context->Unmap(numTraceables.buf.Get(), 0);

	// Also cache the total number of intersections from the last frame (excluding SWR)
	// Needed for calibrating SWR intensity
	D3D11_MAPPED_SUBRESOURCE maxTraceableCount;
	context->CopyResource(maxTraceablesCtrCPU.buf.Get(), maxTraceablesCtr.buf.Get());
	context->Map(maxTraceablesCtrCPU.buf.Get(), 0, D3D11_MAP_READ, 0, &maxTraceableCount);
	fourByteSigned maxTraceableCounter = *(fourByteSigned*)(maxTraceableCount.pData);
	context->Unmap(maxTraceablesCtrCPU.buf.Get(), 0);
	prevNumTraceables = maxTraceableCounter;

	// Create a CPU-accessible reference to the shader input buffer
	D3D11_MAPPED_SUBRESOURCE shaderInput;
	context->Map(renderInputBuffer.buf.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &shaderInput);
	InputStuffs* shaderInputPtr = (InputStuffs*)shaderInput.pData;

	// Calculate a dispatch width
	fourByteUnsigned dispWidth = (fourByteUnsigned)std::ceil(std::cbrt((float)traceableCounter / 128));

	// Ask the standard chronometry library for the current time in seconds
	std::chrono::nanoseconds currTimeNanoSecs = std::chrono::steady_clock::now().time_since_epoch();
	fourByteUnsigned currTime = std::chrono::duration_cast<std::chrono::duration<fourByteUnsigned>>(currTimeNanoSecs).count();

	// Update the GPU-side version of [timeDispInfo]
	shaderInputPtr->timeDispInfo = DirectX::XMFLOAT4(TimeStuff::deltaTime(),
													 (float)currTime,
													 (float)TimeStuff::frameCtr,
													 (float)dispWidth);

	// Update the GPU-side version of [numTraceables]
	shaderInputPtr->numTraceables = DirectX::XMFLOAT4((float)traceableCounter, // Captured above
													  0.0f, // Undefined until the next frame
													  GPGPUStuff::RASTER_CELL_DEPTH,
												      0.0f);

	// Data outside [timeDispInfo] and [prevNumTraceables] will have been destroyed by
	// [MAP_WRITE_DISCARD]; replenish those over here

	// Define the camera position + view-matrix + inverse view-matrix
	shaderInputPtr->cameraPos = cameraPosition;
	shaderInputPtr->viewMat = viewMatrix;
	shaderInputPtr->iViewMat = DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(viewMatrix),
														viewMatrix);
	// Define the number of bounces for each primary ray
	shaderInputPtr->maxNumBounces = DirectX::XMUINT4(GraphicsStuff::MAX_NUM_BOUNCES,
													 GraphicsStuff::MAX_NUM_BOUNCES,
													 GraphicsStuff::MAX_NUM_BOUNCES,
													 GraphicsStuff::MAX_NUM_BOUNCES);
	// Re-fill the GPU-side version of [resInfo]
	shaderInputPtr->resInfo = DirectX::XMUINT4(GraphicsStuff::DISPLAY_WIDTH,
											   GraphicsStuff::DISPLAY_HEIGHT,
											   GraphicsStuff::NUM_AA_SAMPLES,
											   GraphicsStuff::DISPLAY_AREA);

	// Break the write-allowed connection to [shaderInputBuffer]
	context->Unmap(renderInputBuffer.buf.Get(), 0);

	// Dispatch the path tracer
	context->Dispatch(dispWidth,
					  dispWidth,
					  dispWidth);

	// Resources can't be exposed through unordered-access-views (read/write allowed) and shader-
	// resource-views (read-only) simultaneously, so un-bind the local unordered-access-view
	// of the display texture over here
	ID3D11UnorderedAccessView* nullUAV = nullptr;
	context->CSSetUnorderedAccessViews(0, 1, &nullUAV, 0);
}

void Renderer::Present(ViewFinder* viewFinder)
{
	viewFinder->GetViewGeo().PassToGPU(context);
	screenPainter.Render(context,
						 displayInputBuffer.buf,
					     viewFinder->GetDisplayTex().asReadOnlyShaderResource);
}

// Push constructions for this class through Athru's custom allocator
void* Renderer::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<Renderer>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void Renderer::operator delete(void* target)
{
	return;
}