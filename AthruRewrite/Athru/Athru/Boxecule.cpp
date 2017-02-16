#include "assert.h"
#include "Typedefs.h"
#include "Boxecule.h"

Boxecule::Boxecule(ID3D11Device* device, Material mat, SQTTransformer transformStuff) : 
				   material(mat), transformData(transformStuff)
{
	// Cache the material color
	float* colors = material.GetColorData();

	// Generate cubic verts (first four are at Z = 0, second four are at Z = 1)
	Vertex verts[BOXECULE_VERT_COUNT] = { Vertex(0,0,0, colors[0], colors[1], colors[2], colors[3]),
										  Vertex(0,1,0, colors[0], colors[1], colors[2], colors[3]),
										  Vertex(1,1,0, colors[0], colors[1], colors[2], colors[3]),
										  Vertex(1,0,0, colors[0], colors[1], colors[2], colors[3]),
										  Vertex(0,0,1, colors[0], colors[1], colors[2], colors[3]),
										  Vertex(0,1,1, colors[0], colors[1], colors[2], colors[3]),
										  Vertex(1,1,1, colors[0], colors[1], colors[2], colors[3]),
										  Vertex(1,0,1, colors[0], colors[1], colors[2], colors[3]) };

	// Generate vert indices (used to construct the shape, broken into 12 separate triangles)
	// Each set of three items is equivalent to one triangle
	unsigned long indices[36] = { 0,
								  1,
								  2,

								  1,
								  3,
								  2,

								  2,
								  3,
								  4,

								  3,
								  1,
								  4,

								  4,
								  1,
								  5,

								  1,
								  0,
								  5,

								  5,
								  0,
								  6,

								  0,
								  2,
								  6,

								  6,
								  2,
								  7,

								  2,
								  7,
								  4,

								  7,
								  4,
								  6,

								  6,
								  4,
								  5 };

	// Long integer used to store success/failure for different DirectX operations
	HRESULT result;

	// Set up the description of the static vertex buffer
	D3D11_BUFFER_DESC vertBufferDesc;
	vertBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertBufferDesc.ByteWidth = sizeof(Vertex) * BOXECULE_VERT_COUNT;
	vertBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertBufferDesc.CPUAccessFlags = 0;
	vertBufferDesc.MiscFlags = 0;
	vertBufferDesc.StructureByteStride = 0;

	// Give a subresource structure a pointer to the vertex data.
	D3D11_SUBRESOURCE_DATA vertData;
	vertData.pSysMem = verts;
	vertData.SysMemPitch = 0;
	vertData.SysMemSlicePitch = 0;

	// Instantiate the vertex buffer
	result = device->CreateBuffer(&vertBufferDesc, &vertData, &vertBuffer);

	// Set up the description of the static index buffer
	D3D11_BUFFER_DESC indexBufferDesc;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * BOXECULE_VERT_COUNT;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Give a subresource structure a pointer to the index data
	D3D11_SUBRESOURCE_DATA indexData;
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Instantiate the index buffer
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);

	// Break the program if either of the buffers couldn't be created
	assert(SUCCEEDED(result));
}

Boxecule::~Boxecule()
{
	// Release the vertex buffer
	vertBuffer->Release();
	vertBuffer = 0;

	// Release the index buffer
	indexBuffer->Release();
	indexBuffer = 0;
}

void Boxecule::Render(ID3D11DeviceContext* deviceContext)
{
	// Set vertex buffer stride and offset.
	fourByteUnsigned stride = sizeof(Vertex);
	fourByteUnsigned offset = 0;

	// Activate the vertex buffer + index buffer so we can render [this]
	deviceContext->IASetVertexBuffers(0, 1, &vertBuffer, &stride, &offset);
	deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
}
