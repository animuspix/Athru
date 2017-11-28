#include "UtilityServiceCentre.h"
#include "GPUServiceCentre.h"
#include "TracePix.h"
#include "PathTracer.h"

PathTracer::PathTracer(LPCWSTR shaderFilePath) :
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

	// Describe the GI calculation buffer we'll be using
	// to store primary contributions before averaging them
	// during post-processing
	D3D11_BUFFER_DESC giCalcBufferDesc;
	giCalcBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	giCalcBufferDesc.ByteWidth = sizeof(float[4]) * GraphicsStuff::GI_SAMPLE_TOTAL;
	giCalcBufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	giCalcBufferDesc.CPUAccessFlags = 0;
	giCalcBufferDesc.MiscFlags = 0;
	giCalcBufferDesc.StructureByteStride = 0;

	// Instantiate the primary GI calculation buffer from the description
	// we made above
	ID3D11Device* device = AthruGPU::GPUServiceCentre::AccessD3D()->GetDevice();
	HRESULT result = device->CreateBuffer(&giCalcBufferDesc, NULL, &giCalcBuffer);
	assert(SUCCEEDED(result));

	// Describe the the shader-friendly write-only resource view we'll use to
	// access the primary GI calculation buffer during path tracing

	D3D11_BUFFER_UAV giCalcBufferViewADescA;
	giCalcBufferViewADescA.FirstElement = 0;
	giCalcBufferViewADescA.Flags = 0;
	giCalcBufferViewADescA.NumElements = GraphicsStuff::GI_SAMPLE_TOTAL;

	D3D11_UNORDERED_ACCESS_VIEW_DESC giCalcBufferViewADescB;
	giCalcBufferViewADescB.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	giCalcBufferViewADescB.Buffer = giCalcBufferViewADescA;
	giCalcBufferViewADescB.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;

	// Instantiate the primary GI calculation buffer's shader-friendly write-only view from the
	// description we made above
	result = device->CreateUnorderedAccessView(giCalcBuffer, &giCalcBufferViewADescB, &giCalcBufferViewWritable);
	assert(SUCCEEDED(result));

	// Describe the the shader-friendly read-only resource view we'll use to
	// access the primary GI calculation buffer during path tracing

	D3D11_BUFFER_SRV giCalcBufferViewBDescA;
	giCalcBufferViewBDescA.FirstElement = 0;
	giCalcBufferViewBDescA.NumElements = GraphicsStuff::GI_SAMPLE_TOTAL;

	D3D11_SHADER_RESOURCE_VIEW_DESC giCalcBufferViewBDescB;
	giCalcBufferViewBDescB.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	giCalcBufferViewBDescB.Buffer = giCalcBufferViewBDescA;
	giCalcBufferViewBDescB.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;

	// Instantiate the primary GI calculation buffer's shader-friendly read-only view from the
	// description we made above
	result = device->CreateShaderResourceView(giCalcBuffer, &giCalcBufferViewBDescB, &giCalcBufferViewReadable);
	assert(SUCCEEDED(result));

	// Initialise the progressive-render-pass counter variable
	progPassCounter = 0;
}

PathTracer::~PathTracer()
{
	shaderInputBuffer->Release();
	shaderInputBuffer = nullptr;

	giCalcBuffer->Release();
	giCalcBuffer = nullptr;

	giCalcBufferViewWritable->Release();
	giCalcBufferViewWritable = nullptr;

	giCalcBufferViewReadable->Release();
	giCalcBufferViewReadable = nullptr;
}

void PathTracer::Dispatch(ID3D11DeviceContext* context,
						  DirectX::XMVECTOR& cameraPosition,
						  DirectX::XMMATRIX& viewMatrix)
{
	// Most of the shader inputs we expect to use were set during start-up,
	// so no need to set them again over here
	context->CSSetShader(shader, 0, 0);

	// Starting path tracing means passing a write-allowed view of our GI calculation buffer
	// onto the GPU, so do that here
	context->CSSetUnorderedAccessViews(4, 1, &giCalcBufferViewWritable, 0);

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
	context->Dispatch(GraphicsStuff::DISPLAY_WIDTH, GraphicsStuff::GI_SAMPLES_PER_RAY, 1);
}

ID3D11ShaderResourceView* PathTracer::GetGICalcBufferReadable()
{
	return giCalcBufferViewReadable;
}