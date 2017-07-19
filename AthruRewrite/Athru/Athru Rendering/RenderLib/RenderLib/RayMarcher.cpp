#include "UtilityServiceCentre.h"
#include "GPUServiceCentre.h"
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
}

RayMarcher::~RayMarcher()
{
	shaderInputBuffer->Release();
	shaderInputBuffer = nullptr;
}

void RayMarcher::Dispatch(ID3D11DeviceContext* context,
						  DirectX::XMVECTOR& cameraPosition,
						  DirectX::XMMATRIX& viewMatrix,
						  ID3D11ShaderResourceView* gpuReadableSceneDataView,
						  ID3D11UnorderedAccessView* gpuWritableSceneDataView,
						  fourByteUnsigned numFigures)
{
	context->CSSetShader(shader, 0, 0);
	context->CSSetUnorderedAccessViews(0, 1, &displayTexture, 0);
	context->CSSetShaderResources(0, 1, &gpuReadableSceneDataView);
	context->CSSetUnorderedAccessViews(1, 1, &gpuWritableSceneDataView, 0);

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

	// Break the write-allowed connection to the shader input buffer
	context->Unmap(shaderInputBuffer, 0);

	// Pass the data in the local input buffer over to the GPU
	context->CSSetConstantBuffers(0, 1, &shaderInputBuffer);

	// Dispatch the raw shader program associated with [this]
	context->Dispatch(GraphicsStuff::DISPLAY_WIDTH / 4, GraphicsStuff::DISPLAY_HEIGHT / 4, 1);

	// We've finished ray-marching for this frame, so allow the post-processing
	// pass to run by detaching the display texture from [this]
	ID3D11UnorderedAccessView* nullUAV = { NULL };
	context->CSSetUnorderedAccessViews(0, 1, &nullUAV, 0);
}
