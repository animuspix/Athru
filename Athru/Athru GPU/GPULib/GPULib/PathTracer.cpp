#include "UtilityServiceCentre.h"
#include "GPUServiceCentre.h"
#include "PixHistory.h"
#include "PathTracer.h"

PathTracer::PathTracer(LPCWSTR shaderFilePath) :
					   ComputeShader(AthruGPU::GPUServiceCentre::AccessD3D()->GetDevice(),
									 AthruUtilities::UtilityServiceCentre::AccessApp()->GetHWND(),
									 shaderFilePath)
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
	const Microsoft::WRL::ComPtr<ID3D11Device>& device = AthruGPU::GPUServiceCentre::AccessD3D()->GetDevice();
	HRESULT result = device->CreateBuffer(&inputBufferDesc, NULL, shaderInputBuffer.GetAddressOf());
	assert(SUCCEEDED(result));

	// Build the buffer we'll be using to store
	// image samples for temporal
	// smoothing/anti-aliasing
	fourByteUnsigned length = GraphicsStuff::DISPLAY_AREA;
	GPGPUStuff::BuildRWStructBuffer<PixHistory>(device,
												aaBuffer,
												NULL,
												aaBufferView,
												length);
}

PathTracer::~PathTracer()
{
	// Nullify input/AA buffers here
	shaderInputBuffer = nullptr;
	aaBuffer = nullptr;
	aaBufferView = nullptr;
}

void PathTracer::Dispatch(const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
						  DirectX::XMVECTOR& cameraPosition,
						  DirectX::XMMATRIX& viewMatrix,
						  const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& displayTexWritable)
{
	// Most of the shader inputs we expect to use were set during start-up,
	// so no need to set them again over here
	context->CSSetShader(shader.Get(), 0, 0);

	// Path-tracing needs random ray directions, so pass a write-allowed view of our shader-friendly
	// random-number buffer onto the GPU over here
	const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& gpuRandView = AthruGPU::GPUServiceCentre::AccessGPURandView();
	context->CSSetUnorderedAccessViews(2, 1, gpuRandView.GetAddressOf(), 0);

	// This render-stage publishes to the screen, so we need to pass the
	// display texture along to the GPU
	context->CSSetUnorderedAccessViews(1, 1, displayTexWritable.GetAddressOf(), 0);

	// We want to perform stratified anti-aliasing (basically tracing pseudo-random
	// subpixel rays each frame) and integrate the results over time, so push
	// the anti-aliasing buffer onto the GPU as well
	context->CSSetUnorderedAccessViews(3, 1, aaBufferView.GetAddressOf(), 0);

	// Expose the local input buffer for writing
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT result = context->Map(shaderInputBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	assert(SUCCEEDED(result));

	// Get a pointer to the raw data in the shader input buffer
	InputStuffs* dataPtr;
	dataPtr = (InputStuffs*)mappedResource.pData;

	// Copy in the camera position + view matrix (also the inverse view matrix)
	dataPtr->cameraPos = cameraPosition;
	dataPtr->viewMat = viewMatrix;
	dataPtr->iViewMat = DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(viewMatrix),
												 viewMatrix);

	// Copy in the delta-time value at the current frame
	float t = TimeStuff::deltaTime();
	dataPtr->deltaTime = DirectX::XMFLOAT4(t, t, t, t);

	// Ask the standard chronometry library for the current time
	// in seconds, then pass the value it returns into the input
	// buffer
	std::chrono::nanoseconds currTimeNanoSecs = std::chrono::steady_clock::now().time_since_epoch();
	fourByteUnsigned currTime = std::chrono::duration_cast<std::chrono::duration<fourByteUnsigned>>(currTimeNanoSecs).count();
	dataPtr->currTimeSecs = DirectX::XMUINT4(currTime, currTime, currTime, currTime);

	// Write the ID of the current progressive-rendering pass into the
	// local input buffer
	dataPtr->rendPassID = DirectX::XMUINT4(progPassCounters.x, progPassCounters.y, 0, 0);

	// Define the number of 8x8 render-slices in each pass
	dataPtr->numProgPatches = DirectX::XMUINT4(GraphicsStuff::PROG_PATCHES_PER_FRAME,
											  0, 0, 0);

	// Define the number of bounces for each primary ray
	dataPtr->maxNumBounces = DirectX::XMUINT4(GraphicsStuff::MAX_NUM_BOUNCES,
											  GraphicsStuff::MAX_NUM_BOUNCES,
											  GraphicsStuff::MAX_NUM_BOUNCES,
											  GraphicsStuff::MAX_NUM_BOUNCES);

	// Define the number of direct gather-rays (area-light samples) in each bounce
	dataPtr->numDirGaths = DirectX::XMUINT4(GraphicsStuff::NUM_DIRECT_SAMPLES,
											GraphicsStuff::NUM_DIRECT_SAMPLES,
											GraphicsStuff::NUM_DIRECT_SAMPLES,
											GraphicsStuff::NUM_DIRECT_SAMPLES);

	// Define the number of indirect gather-rays (ambient samples) in each bounce
	dataPtr->numIndirGaths = DirectX::XMUINT4(GraphicsStuff::NUM_INDIRECT_SAMPLES,
											  GraphicsStuff::NUM_INDIRECT_SAMPLES,
											  GraphicsStuff::NUM_INDIRECT_SAMPLES,
											  GraphicsStuff::NUM_INDIRECT_SAMPLES);

	// Break the write-allowed connection to the shader input buffer
	context->Unmap(shaderInputBuffer.Get(), 0);

	// Pass the data in the local input buffer over to the GPU
	context->CSSetConstantBuffers(0, 1, shaderInputBuffer.GetAddressOf());

	// Increment the render-pass counter so that each frame will render left-to-right
	// and bottom-to-top
	progPassCounters.x = (progPassCounters.x + 1); // Increment [x]
	progPassCounters.y += (progPassCounters.x == GraphicsStuff::PROG_PASS_COUNT_X); // Only increment [y] if [x] wraps over from the right

	// Lock the [y] and [x] aspects of the render-pass counter to the intervals [0...MAX_PASSES_Y] and [0...MAX_PASSES_X] respectively
	progPassCounters.y *= (progPassCounters.y != GraphicsStuff::PROG_PASS_COUNT_Y);
	progPassCounters.x *= (progPassCounters.x != GraphicsStuff::PROG_PASS_COUNT_X);

	// Dispatch the path tracer
	double cubeRt = std::cbrt(GraphicsStuff::PROG_PATCHES_PER_FRAME);
	if (std::floor(cubeRt) == cubeRt)
	{
		// The number of patches-per-frame is perfectly cubic, so we can evenly fill each axis
		context->Dispatch((fourByteUnsigned)cubeRt,
						  (fourByteUnsigned)cubeRt,
						  (fourByteUnsigned)cubeRt);
	}

	else
	{
		// The number of patches-per-frame has no rational cube root; treat it as a square
		// instead (patches-per-frame are required to be /at least/ square in order to cleanly
		// tile the image)
		double squaRt = std::sqrt(GraphicsStuff::PROG_PATCHES_PER_FRAME);
		context->Dispatch((fourByteUnsigned)squaRt,
						  (fourByteUnsigned)squaRt,
						  1);
	}

	// We've finished initial rendering for this frame, so allow the presentation
	// pass to run by detaching the screen texture's unordered-access-view from the
	// GPU's compute shader state
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> nullUAV = nullptr;
	context->CSSetUnorderedAccessViews(1, 1, &nullUAV, 0);
}