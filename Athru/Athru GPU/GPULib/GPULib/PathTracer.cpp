#include "UtilityServiceCentre.h"
#include "GPUServiceCentre.h"
#include "PathTracer.h"
#include "PixHistory.h"

PathTracer::PathTracer(LPCWSTR sceneVisFilePath,
					   LPCWSTR pathReduceFilePath,
					   HWND windowHandle,
					   const Microsoft::WRL::ComPtr<ID3D11Device>& device) :
			tracers{ LocalComputeShader(device,
										windowHandle,
										pathReduceFilePath),
					 LocalComputeShader(device,
					 					windowHandle,
					 					sceneVisFilePath) }
{
	// Describe the shader input buffer we'll be using
	// for shader i/o operations through [this]
	D3D11_BUFFER_DESC inputBufferDesc;
	inputBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	inputBufferDesc.ByteWidth = sizeof(InputStuffs);
	inputBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	inputBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	inputBufferDesc.MiscFlags = 0;
	inputBufferDesc.StructureByteStride = 0;

	// Instantiate the input buffer from the description
	// we made above
	HRESULT result = device->CreateBuffer(&inputBufferDesc, NULL, shaderInputBuffer.GetAddressOf());
	assert(SUCCEEDED(result));

	// Build the buffer we'll be using to store
	// image samples for temporal
	// smoothing/anti-aliasing
	fourByteUnsigned length = GraphicsStuff::DISPLAY_AREA;
	GPGPUStuff::BuildRWStructBuffer<PixHistory>(device,
												aaBuffer,
												nullptr,
												aaBufferView,
												(D3D11_BUFFER_UAV_FLAG)0,
												length);
	// Create the intersection buffer
	// [traceables] are [float2x3]'s in shader code, so no reason to use
	// a fancy globally-visible discrete type when a small temporary
	// wrapper around an array of [XMFLOAT3]s will work too
	struct float2x3 { DirectX::XMFLOAT3 rows[2]; };
	GPGPUStuff::BuildRWStructBuffer<float2x3>(device,
											  traceables,
											  nullptr,
											  traceablesAppendView,
											  D3D11_BUFFER_UAV_FLAG_APPEND,
											  GraphicsStuff::DISPLAY_AREA);

	// Create the buffer we'll use to capture [traceables]' hidden counter
	// value
	GPGPUStuff::BuildStagingBuffer<fourByteUnsigned>(device,
													 numTraceables,
													 1);

	// Create the duplicate intersection buffer
	GPGPUStuff::BuildRWStructBuffer<fourByteUnsigned>(device,
													  maxTraceables,
													  nullptr,
													  maxTraceablesAppendView,
													  D3D11_BUFFER_UAV_FLAG_APPEND,
													  GraphicsStuff::DISPLAY_AREA);

	// Create the counter buffer associated with [maxTraceables]
	GPGPUStuff::BuildStagingBuffer<fourByteUnsigned>(device,
												     maxNumTraceables,
													 1);

	// Initially zero [prevNumTraceables]
	prevNumTraceables = 0;
}

PathTracer::~PathTracer() {}

void PathTracer::PreProcess(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
							const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& displayTexWritable,
							DirectX::XMVECTOR& cameraPosition,
							DirectX::XMMATRIX& viewMatrix)
{
	// Pass the pre-processing shader onto the GPU
	context->CSSetShader(tracers[0].d3dShader().Get(), nullptr, 0);

	// Pass all the path-tracing elements we expect to use onto the GPU as well
	// Path-tracing needs random ray directions, so pass a write-allowed view of our shader-friendly
	// random-number buffer onto the GPU over here
	const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& gpuRandView = AthruGPU::GPUServiceCentre::AccessGPURandView();
	context->CSSetUnorderedAccessViews(2, 1, gpuRandView.GetAddressOf(), 0);

	// This render-stage publishes to the screen, so we need to pass the
	// display texture along to the GPU
	context->CSSetUnorderedAccessViews(1, 1, displayTexWritable.GetAddressOf(), 0);

	// ...and it also exposes selected pixels to the path-tracer via [traceables], so we need to
	// pass that to the GPU as well
	context->CSSetUnorderedAccessViews(4, 1, traceablesAppendView.GetAddressOf(), 0);

	// We want to perform stratified anti-aliasing (basically tracing pseudo-random
	// subpixel rays each frame) and integrate the results over time, so push
	// the anti-aliasing buffer onto the GPU as well
	context->CSSetUnorderedAccessViews(3, 1, aaBufferView.GetAddressOf(), 0);

	// Expose the local input buffer for writing
	D3D11_MAPPED_SUBRESOURCE shaderInput;
	HRESULT result = context->Map(shaderInputBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &shaderInput);
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
													 0.0f, // Number of traceables remaining from the previous frame
													 GraphicsStuff::DISPLAY_AREA / GraphicsStuff::GROUP_AREA_PATH_REDUCTION); // Total group count (preprocessing)

	// Update the GPU-side version of [prevNumTraceables]
	shaderInputPtr->prevNumTraceables = DirectX::XMFLOAT4((float)prevNumTraceables,
														  (float)prevNumTraceables,
														  (float)prevNumTraceables,
													      (float)prevNumTraceables);

	// Break the write-allowed connection to the shader input buffer
	context->Unmap(shaderInputBuffer.Get(), 0);

	// Pass the data in the local input buffer over to the GPU
	context->CSSetConstantBuffers(0, 1, shaderInputBuffer.GetAddressOf());

	// Dispatch the pre-processing shader
	context->Dispatch(GraphicsStuff::DISPLAY_WIDTH / GraphicsStuff::GROUP_WIDTH_PATH_REDUCTION,
					  GraphicsStuff::DISPLAY_HEIGHT / GraphicsStuff::GROUP_WIDTH_PATH_REDUCTION,
					  1);
}

void PathTracer::Dispatch(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
						  DirectX::XMVECTOR& cameraPosition,
						  DirectX::XMMATRIX& viewMatrix)
{
	// Most of the shader inputs we expect to use were set during pre-processing, so no reason to
	// set them again over here
	context->CSSetShader(tracers[1].d3dShader().Get(), nullptr, 0);

	// ...but we will have (usually) [Append()]'d some data to [traceables] during pre-processing,
	// so read the length of that back into a local value here
	D3D11_MAPPED_SUBRESOURCE traceableCount;
	context->CopyStructureCount(numTraceables.Get(), 0, traceablesAppendView.Get());
	context->Map(numTraceables.Get(), 0, D3D11_MAP_READ, 0, &traceableCount);
	fourByteUnsigned traceableCounter = *(fourByteUnsigned*)(traceableCount.pData);
	context->Unmap(numTraceables.Get(), 0);

	// Also cache the total number of intersections from the last frame (excluding SWR)
	// Needed for calibrating SWR intensity
	D3D11_MAPPED_SUBRESOURCE maxTraceableCount;
	context->CopyStructureCount(maxNumTraceables.Get(), 0, maxTraceablesAppendView.Get());
	context->Map(maxNumTraceables.Get(), 0, D3D11_MAP_READ, 0, &maxTraceableCount);
	fourByteUnsigned maxTraceableCounter = *(fourByteUnsigned*)(maxTraceableCount.pData);
	context->Unmap(maxNumTraceables.Get(), 0);
	prevNumTraceables = maxTraceableCounter;

	// Create a CPU-accessible reference to the shader input buffer
	D3D11_MAPPED_SUBRESOURCE shaderInput;
	context->Map(shaderInputBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &shaderInput);
	InputStuffs* shaderInputPtr = (InputStuffs*)shaderInput.pData;
	
	// Calculate a dispatch width
	fourByteUnsigned dispWidth = (fourByteUnsigned)std::ceil(std::cbrt((float)traceableCounter / 128));

	// Ask the standard chronometry library for the current time in seconds
	std::chrono::nanoseconds currTimeNanoSecs = std::chrono::steady_clock::now().time_since_epoch();
	fourByteUnsigned currTime = std::chrono::duration_cast<std::chrono::duration<fourByteUnsigned>>(currTimeNanoSecs).count();

	// Update the GPU-side version of [timeDispInfo]
	shaderInputPtr->timeDispInfo = DirectX::XMFLOAT4(TimeStuff::deltaTime(),
													 (float)currTime,
													 (float)traceableCounter,
													 (float)dispWidth);

	// Pass filler values into [prevNumTraceables]
	shaderInputPtr->prevNumTraceables = DirectX::XMFLOAT4(0.0f,
														  0.0f,
														  0.0f,
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

	// Break the write-allowed connection to [shaderInputBuffer]
	context->Unmap(shaderInputBuffer.Get(), 0);

	// Dispatch the path tracer
	context->Dispatch(dispWidth,
					  dispWidth,
					  dispWidth);

	// Resources can't be exposed through unordered-access-views (read/write allowed) and shader-
	// resource-views (read-only) simultaneously, so un-bind the local unordered-access-view
	// of the display texture over here
	context->CSSetUnorderedAccessViews(1, 1, &nullUAV, 0);
}

// Push constructions for this class through Athru's custom allocator
void* PathTracer::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<PathTracer>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void PathTracer::operator delete(void* target)
{
	return;
}