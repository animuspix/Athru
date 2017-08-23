#include "GPUServiceCentre.h"
#include "PathTracer.h"

PathTracer::PathTracer(LPCWSTR shaderFilePath) :
			ComputeShader(AthruGPU::GPUServiceCentre::AccessD3D()->GetDevice(),
						  AthruUtilities::UtilityServiceCentre::AccessApp()->GetHWND(),
						  shaderFilePath)
{
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

	// Instantiate the primary GI calculation buffer's shader-friendly write-only view from the
	// description we made above
	result = device->CreateShaderResourceView(giCalcBuffer, &giCalcBufferViewBDescB, &giCalcBufferViewReadable);
	assert(SUCCEEDED(result));
}

PathTracer::~PathTracer()
{
	giCalcBuffer->Release();
	giCalcBuffer = nullptr;

	giCalcBufferViewWritable->Release();
	giCalcBufferViewWritable = nullptr;

	giCalcBufferViewReadable->Release();
	giCalcBufferViewReadable = nullptr;
}

void PathTracer::Dispatch(ID3D11DeviceContext* context)
{
	// Most of the shader inputs we expect to use were already set by the ray-marcher,
	// so no need to set them again over here
	context->CSSetShader(shader, 0, 0);

	// Starting path tracing means passing a write-allowed view of our GI calculation buffer
	// onto the GPU, so do that here
	context->CSSetUnorderedAccessViews(4, 1, &giCalcBufferViewWritable, 0);

	// We can't effectively deploy more than 64 threads in each group, so we're
	// making up the difference by deploying four times as many work groups and
	// integrating as if we were tracing 256 (4 * 64) simultaneous secondary
	// rays (iirc 256 is the upper limit for sensible path-tracing sample
	// counts)
	context->Dispatch(GraphicsStuff::MAX_TRACE_COUNT / 4, 16, 1);
}

ID3D11ShaderResourceView* PathTracer::GetGICalcBufferReadable()
{
	return giCalcBufferViewReadable;
}
