#include "UtilityServiceCentre.h"
#include "GPUServiceCentre.h"
#include "TracePoint.h"
#include "RayMarcher.h"

RayMarcher::RayMarcher(LPCWSTR shaderFilePath) :
					   ComputeShader(AthruGPU::GPUServiceCentre::AccessD3D()->GetDevice(),
									 AthruUtilities::UtilityServiceCentre::AccessApp()->GetHWND(),
									 shaderFilePath)
{
	TextureManager* localTextureManagerPttr = AthruGPU::GPUServiceCentre::AccessTextureManager();
	AthruTexture2D displayTex = localTextureManagerPttr->GetDisplayTexture(AVAILABLE_DISPLAY_TEXTURES::SCREEN_TEXTURE);
	displayTexture = displayTex.asWritableShaderResource;

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
	ID3D11Device* device = AthruGPU::GPUServiceCentre::AccessD3D()->GetDevice();
	HRESULT result = device->CreateBuffer(&inputBufferDesc, NULL, &shaderInputBuffer);
	assert(SUCCEEDED(result));

	// Describe the primary trace buffer we'll be using
	// to store intersections before integrating their
	// local illumination during path tracing
	D3D11_BUFFER_DESC traceBufferDesc;
	traceBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	traceBufferDesc.ByteWidth = sizeof(TracePoint) * GraphicsStuff::MAX_TRACE_COUNT;
	traceBufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	traceBufferDesc.CPUAccessFlags = 0;
	traceBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	traceBufferDesc.StructureByteStride = sizeof(TracePoint);

	// Avoid awkward post-processing issues by seeding
	// the trace buffer with an array of initialized
	// members (GI application will produce unexpected
	// results in the zeroth frame if each member of
	// the trace buffer is left uninitialized)
	TracePoint* traceables = new TracePoint[GraphicsStuff::MAX_TRACE_COUNT];
	for (fourByteUnsigned i = 0; i < GraphicsStuff::MAX_TRACE_COUNT; i += 1)
	{
		traceables[i].coord = _mm_set_ps(1.0f, 0, 0, 0);
		traceables[i].rgbaSrc = _mm_set_ps(1.0f, 0, 0, 0);
		traceables[i].figID = DirectX::XMUINT4(0, 0, 0, 0);
		traceables[i].isValid = DirectX::XMUINT4(0, 0, 0, 0);
		traceables[i].rgbaGI = _mm_set_ps(1.0f, 0, 0, 0);
	}

	D3D11_SUBRESOURCE_DATA traceBufInit;
	traceBufInit.pSysMem = traceables;
	traceBufInit.SysMemPitch = 0;
	traceBufInit.SysMemSlicePitch = 0;

	// Instantiate the primary trace buffer from the description
	// we made above
	result = device->CreateBuffer(&traceBufferDesc, NULL, &traceBuffer);
	assert(SUCCEEDED(result));

	// No more need for the CPU-side trace-point array, so delete
	// it here
	delete traceables;
	traceables = nullptr;

	// Describe the the shader-friendly resource view we'll use to
	// access the primary trace buffer during path tracing

	D3D11_BUFFER_UAV traceBufferViewDescA;
	traceBufferViewDescA.FirstElement = 0;
	traceBufferViewDescA.Flags = 0;
	traceBufferViewDescA.NumElements = GraphicsStuff::MAX_TRACE_COUNT;

	D3D11_UNORDERED_ACCESS_VIEW_DESC traceBufferViewDescB;
	traceBufferViewDescB.Format = DXGI_FORMAT_UNKNOWN;
	traceBufferViewDescB.Buffer = traceBufferViewDescA;
	traceBufferViewDescB.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;

	// Instantiate the primary trace buffer's shader-friendly view from the
	// description we made above
	result = device->CreateUnorderedAccessView(traceBuffer, &traceBufferViewDescB, &traceBufferView);
	assert(SUCCEEDED(result));

	// Initialise the progressive-render-pass counter variable
	progPassCounter = 0;
}

RayMarcher::~RayMarcher()
{
	shaderInputBuffer->Release();
	shaderInputBuffer = nullptr;

	traceBuffer->Release();
	traceBuffer = nullptr;

	traceBufferView->Release();
	traceBufferView = nullptr;
}

void RayMarcher::Dispatch(ID3D11DeviceContext* context,
						  DirectX::XMVECTOR& cameraPosition,
						  DirectX::XMMATRIX& viewMatrix,
						  ID3D11ShaderResourceView* gpuReadableSceneDataView,
						  ID3D11UnorderedAccessView* gpuWritableSceneDataView,
						  ID3D11UnorderedAccessView* gpuRandView,
						  fourByteUnsigned numFigures)
{
	context->CSSetShader(shader, 0, 0);
	context->CSSetUnorderedAccessViews(2, 1, &displayTexture, 0);
	context->CSSetShaderResources(0, 1, &gpuReadableSceneDataView);
	context->CSSetUnorderedAccessViews(0, 1, &gpuWritableSceneDataView, 0);
	context->CSSetUnorderedAccessViews(1, 1, &gpuRandView, 0);
	context->CSSetUnorderedAccessViews(3, 1, &traceBufferView, 0);

	// Expose the local input buffer for writing
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT result = context->Map(shaderInputBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	assert(SUCCEEDED(result));

	// Get a pointer to the raw data in the shader input buffer
	InputStuffs* dataPtr;
	dataPtr = (InputStuffs*)mappedResource.pData;

	// Copy in the camera position + view matrix
	dataPtr->cameraPos = cameraPosition;
	dataPtr->viewMat = viewMatrix;

	// Copy in the delta-time value at the current frame
	float t = TimeStuff::deltaTime();
	dataPtr->deltaTime = DirectX::XMFLOAT4(t, t, t, t);

	// Ask the standard chronometry library for the current time
	// in seconds, then pass the value it returns into the input
	// buffer
	std::chrono::nanoseconds currTimeNanoSecs = std::chrono::steady_clock::now().time_since_epoch();
	fourByteUnsigned currTime = std::chrono::duration_cast<std::chrono::duration<fourByteUnsigned>>(currTimeNanoSecs).count();
	dataPtr->currTimeSecs = DirectX::XMUINT4(currTime, currTime, currTime, currTime);

	// Write the given figure-count into the local input buffer
	dataPtr->numFigures = DirectX::XMUINT4(numFigures, numFigures, numFigures, numFigures);

	// Write the ID of the current progressive-rendering pass into the
	// local input buffer
	dataPtr->rendPassID = DirectX::XMUINT4(progPassCounter, progPassCounter, progPassCounter, progPassCounter);

	// Break the write-allowed connection to the shader input buffer
	context->Unmap(shaderInputBuffer, 0);

	// Pass the data in the local input buffer over to the GPU
	context->CSSetConstantBuffers(0, 1, &shaderInputBuffer);

	// Increment the render-pass counter so that the next frame will render
	// the pixel row just below the current one
	progPassCounter += 1;

	// Re-set the render-pass counter if it travels pass the maximum
	// number of progressive passes
	if (progPassCounter > GraphicsStuff::PROG_PASS_COUNT)
	{
		progPassCounter = 0;
	}

	// Dispatch the raw shader program associated with [this]
	context->Dispatch(GraphicsStuff::DISPLAY_WIDTH, 1, 1);
}

ID3D11UnorderedAccessView* RayMarcher::GetTraceBuffer()
{
	return traceBufferView;
}
