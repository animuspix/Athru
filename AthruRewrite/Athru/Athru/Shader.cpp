#include <assert.h>
#include "Typedefs.h"
#include "ServiceCentre.h"
#include "Shader.h"

Shader::Shader(ID3D11Device* device, HWND windowHandle,
			   LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath)
{
	// Long integer used to store success/failure for DirectX operations
	HRESULT result;

	// Read the vertex shader into a buffer on the GPU
	ID3D10Blob* vertShaderBuffer;
	D3DReadFileToBlob(vertexShaderFilePath, &vertShaderBuffer);

	// Read the pixel shader into a buffer on the GPU
	ID3D10Blob* pixelShaderBuffer;
	D3DReadFileToBlob(pixelShaderFilePath, &pixelShaderBuffer);

	// Create the vertex shader from the vertex shader buffer
	result = device->CreateVertexShader(vertShaderBuffer->GetBufferPointer(), vertShaderBuffer->GetBufferSize(), NULL, &vertShader);

	// Create the pixel shader from the pixel shader buffer
	result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pixelShader);

	// Set up the vertex input layout description
	// This setup needs to match the Vertex struct in Boxecule + each shader
	D3D11_INPUT_ELEMENT_DESC polygonLayout[VERTEX_PROPERTY_COUNT];
	polygonLayout[0].SemanticName = "POSITION";
	polygonLayout[0].SemanticIndex = 0;
	polygonLayout[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[0].InputSlot = 0;
	polygonLayout[0].AlignedByteOffset = 0;
	polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[0].InstanceDataStepRate = 0;

	polygonLayout[1].SemanticName = "POSITION";
	polygonLayout[1].SemanticIndex = 1;
	polygonLayout[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[1].InputSlot = 0;
	polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[1].InstanceDataStepRate = 0;

	polygonLayout[2].SemanticName = "COLOR";
	polygonLayout[2].SemanticIndex = 0;
	polygonLayout[2].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[2].InputSlot = 0;
	polygonLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[2].InstanceDataStepRate = 0;

	polygonLayout[3].SemanticName = "COLOR";
	polygonLayout[3].SemanticIndex = 1;
	polygonLayout[3].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[3].InputSlot = 0;
	polygonLayout[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[3].InstanceDataStepRate = 0;

	polygonLayout[4].SemanticName = "NORMAL";
	polygonLayout[4].SemanticIndex = 0;
	polygonLayout[4].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[4].InputSlot = 0;
	polygonLayout[4].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[4].InstanceDataStepRate = 0;

	polygonLayout[5].SemanticName = "NORMAL";
	polygonLayout[5].SemanticIndex = 1;
	polygonLayout[5].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[5].InputSlot = 0;
	polygonLayout[5].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[5].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[5].InstanceDataStepRate = 0;

	polygonLayout[6].SemanticName = "TEXCOORD";
	polygonLayout[6].SemanticIndex = 0;
	polygonLayout[6].Format = DXGI_FORMAT_R32G32_FLOAT;
	polygonLayout[6].InputSlot = 0;
	polygonLayout[6].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[6].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[6].InstanceDataStepRate = 0;

	polygonLayout[7].SemanticName = "COLOR";
	polygonLayout[7].SemanticIndex = 2;
	polygonLayout[7].Format = DXGI_FORMAT_R32_FLOAT;
	polygonLayout[7].InputSlot = 0;
	polygonLayout[7].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[7].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[7].InstanceDataStepRate = 0;

	polygonLayout[8].SemanticName = "COLOR";
	polygonLayout[8].SemanticIndex = 3;
	polygonLayout[8].Format = DXGI_FORMAT_R32_FLOAT;
	polygonLayout[8].InputSlot = 0;
	polygonLayout[8].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[8].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[8].InstanceDataStepRate = 0;

	// Create the vertex input layout
	result = device->CreateInputLayout(polygonLayout, VERTEX_PROPERTY_COUNT, vertShaderBuffer->GetBufferPointer(),
									   vertShaderBuffer->GetBufferSize(), &inputLayout);

	// Release the vertex shader buffer + pixel shader buffer
	vertShaderBuffer->Release();
	vertShaderBuffer = nullptr;

	pixelShaderBuffer->Release();
	pixelShaderBuffer = nullptr;

	// Set up the description of the dynamic matrix cbuffer in the vertex shader.
	D3D11_BUFFER_DESC matrixBufferDesc;
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatBuffer);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;

	// Create a pointer to the local matrix buffer so we can easily move the buffered
	// matrices into the vertex shader's cbuffer
	result = device->CreateBuffer(&matrixBufferDesc, NULL, &matBufferLocal);

	// Initialise the time-accumulator to [0]
	dtAccumulator = 0;

	// Test whether anything broke :P
	assert(SUCCEEDED(result));
}

Shader::~Shader()
{
	// Release the cbuffer
	matBufferLocal->Release();
	matBufferLocal = nullptr;

	// Release the vertex input layout
	inputLayout->Release();
	inputLayout = nullptr;

	// Release the pixel shader
	pixelShader->Release();
	pixelShader = nullptr;

	// Release the vertex shader
	vertShader->Release();
	vertShader = nullptr;

	// Flush any pipeline data associated with [this]
	//ServiceCentre::AccessGraphics()->GetD3D()->GetDeviceContext()->ClearState();
	//ServiceCentre::AccessGraphics()->GetD3D()->GetDeviceContext()->Flush();
}

void Shader::SetShaderParameters(ID3D11DeviceContext* deviceContext,
								  DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
{
	// Long integer used to store success/failure for DirectX operations
	HRESULT result;

	// Transpose the matrices to prepare them for the shader
	world = XMMatrixTranspose(world);
	view = XMMatrixTranspose(view);
	projection = XMMatrixTranspose(projection);

	// Increment/decrement the time accumulator
	// Lots of magic calculus here; probably a good idea to ask someone
	// what's actually happening before I spend too much time
	// documenting it :P
	dtAccumulator += ((TimeStuff::deltaTime() * TimeStuff::deltaTime()) * (float)accumulatorGrowth);

	// If more time has passed than the animation's duration, allow the
	// accumulator to decrement back towards zero; when the value of the
	// accumulator is less than or equal to zero, set the accumulator to
	// begin incrementing back up towards the animation's duration
	if (dtAccumulator > ANIM_DURATION_SECONDS)
	{
		accumulatorGrowth = ACCUMULATOR_DIRECTION::FALLING;
	}

	else if (dtAccumulator <= 0)
	{
		accumulatorGrowth = ACCUMULATOR_DIRECTION::RISING;
	}

	// Expose the local matrix buffer for writing
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	result = deviceContext->Map(matBufferLocal, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

	// Get a pointer to the data in the local matrix buffer
	MatBuffer* dataPtr;
	dataPtr = (MatBuffer*)mappedResource.pData;

	// Copy the world/view/projection matrices into the local matrix buffer
	dataPtr->world = world;
	dataPtr->view = view;
	dataPtr->projection = projection;

	// Also extract the current animation time-step and store it in the
	// local matrix buffer
	dataPtr->animTimeStep = DirectX::XMFLOAT4(dtAccumulator / ANIM_DURATION_SECONDS,
											  dtAccumulator / ANIM_DURATION_SECONDS,
											  dtAccumulator / ANIM_DURATION_SECONDS,
											  dtAccumulator / ANIM_DURATION_SECONDS);

	// Break the write-allowed connection to the local matrix buffer
	deviceContext->Unmap(matBufferLocal, 0);

	// Transfer the data in the local matrix buffer to the cbuffer within the
	// vertex shader
	deviceContext->VSSetConstantBuffers(0, 1, &matBufferLocal);

	// Test whether anything broke :P
	assert(SUCCEEDED(result));
}

void Shader::RenderShader(ID3D11DeviceContext* deviceContext, fourByteUnsigned numIndicesDrawing)
{
	// Set the vertex input layout.
	deviceContext->IASetInputLayout(inputLayout);

	// Set the vertex/pixel/compute shaders that will be used to render the boxecule
	deviceContext->VSSetShader(vertShader, NULL, 0);
	deviceContext->PSSetShader(pixelShader, NULL, 0);

	// Render a boxecule
	deviceContext->DrawIndexed(numIndicesDrawing, 0, 0);
}

void Shader::Render(ID3D11DeviceContext* deviceContext,
					DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection,
					fourByteUnsigned numIndicesDrawing)
{
	SetShaderParameters(deviceContext, world, view, projection);
	RenderShader(deviceContext, numIndicesDrawing);
}

// Push constructions for this class through Athru's custom allocator
void* Shader::operator new(size_t size)
{
	StackAllocator* allocator = ServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<Shader>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void Shader::operator delete(void* target)
{
	return;
}
