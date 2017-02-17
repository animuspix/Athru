#include "assert.h"
#include "Shaders.h"

Shaders::Shaders(ID3D11Device* device, HWND windowHandle)
{
	HRESULT result;
	ID3D10Blob* errorMessage;
	ID3D10Blob* vertPlotterBuffer;
	ID3D10Blob* colorizerBuffer;
	D3D11_INPUT_ELEMENT_DESC polygonLayout[2];
	unsigned int numElements;
	D3D11_BUFFER_DESC matrixBufferDesc;

	// Read the vertex shader into Athru
	D3DReadFileToBlob(L"VertPlotter.cso", &vertPlotterBuffer);

	// Read the pixel shader into Athru
	D3DReadFileToBlob(L"Colorizer.cso", &colorizerBuffer);

	// Create the vertex shader from the buffer
	result = device->CreateVertexShader(vertPlotterBuffer->GetBufferPointer(), vertPlotterBuffer->GetBufferSize(), NULL, &vertPlotter);

	// Create the pixel shader from the buffer
	result = device->CreatePixelShader(colorizerBuffer->GetBufferPointer(), colorizerBuffer->GetBufferSize(), NULL, &colorizer);

	// Create the vertex input layout description.
	// This setup needs to match the Vertex stuct in the ModelClass and in the shader.
	polygonLayout[0].SemanticName = "POSITION";
	polygonLayout[0].SemanticIndex = 0;
	polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[0].InputSlot = 0;
	polygonLayout[0].AlignedByteOffset = 0;
	polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[0].InstanceDataStepRate = 0;

	polygonLayout[1].SemanticName = "COLOR";
	polygonLayout[1].SemanticIndex = 0;
	polygonLayout[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[1].InputSlot = 0;
	polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[1].InstanceDataStepRate = 0;

	assert(SUCCEEDED(result));
}

Shaders::~Shaders()
{

}

void Shaders::Render(ID3D11DeviceContext* deviceContext, 
					 DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
{
	SetShaderParameters(deviceContext, world, view, projection);
	RenderShader(deviceContext);
}