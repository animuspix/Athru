#include "ServiceCentre.h"
#include "AthruRect.h"

AthruRect::AthruRect(Material rectMaterial, 
					 float baseWidth, float baseHeight)
{
	// Long integer used to represent success/failure for different DirectX operations
	HRESULT result;

	// Initialise the material associated with [this]
	material = rectMaterial;

	// Cache the color associated with [this]
	float* color = material.GetColorData();

	// Initialise vertices
	// X and Y coordinates look a helluva lot like magic numbers; they were basically
	// chosen out of trial and error while I was looking for points that'd line up
	// into a reasonable cube on a 16:9 screen without too much distortion
	Vertex vertices[4] = { Vertex((baseWidth / 2) * -1, baseHeight / 2, 0.0f,
								   color), // Front plane, upper left (v0)

						   Vertex(baseWidth / 2, baseHeight / 2, 0.0f,
								  color), // Front plane, upper right (v1)

						   Vertex((baseWidth / 2) * -1, (baseHeight / 2) * -1, 0.0f,
								   color), // Front plane, lower left (v2)

						   Vertex(baseWidth / 2, (baseHeight / 2) * -1, 0.0f,
								  color), }; // Front plane, lower right (v3)

	// Initialise indices
	// Each set of three values is one triangle
	long indices[RECT_INDEX_COUNT] = { 0,
									   1,
									   2,
									   
									   2,
									   1,
									   3 };


	// Set up the description of the static vertex buffer
	D3D11_BUFFER_DESC vertBufferDesc;
	vertBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertBufferDesc.ByteWidth = sizeof(Vertex) * 4;
	vertBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertBufferDesc.CPUAccessFlags = 0;
	vertBufferDesc.MiscFlags = 0;
	vertBufferDesc.StructureByteStride = 0;

	// Place a reference to the vertices in a subresource structure
	D3D11_SUBRESOURCE_DATA vertData;
	vertData.pSysMem = vertices;
	vertData.SysMemPitch = 0;
	vertData.SysMemSlicePitch = 0;

	// Create the vertex buffer
	ID3D11Device* device = ServiceCentre::AccessGraphics()->GetD3D()->GetDevice();
	result = device->CreateBuffer(&vertBufferDesc, &vertData, &vertBuffer);

	// Set up the description of the static index buffer
	D3D11_BUFFER_DESC indexBufferDesc;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * RECT_INDEX_COUNT;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Place a reference to the vertex indices in a subresource structure
	D3D11_SUBRESOURCE_DATA indexData;
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);

	// Check if anything failed during the geometry setup
	assert(SUCCEEDED(result));

	// Initialise the scale, rotation, and position of [this]
	transformations = SQT();
}

AthruRect::~AthruRect()
{
	// Release the index buffer
	indexBuffer->Release();
	indexBuffer = 0;

	// Release the vertex buffer
	vertBuffer->Release();
	vertBuffer = 0;
}

void AthruRect::PassToGPU(ID3D11DeviceContext* deviceContext)
{
	// Set vertex buffer stride and offset.
	unsigned int stride = sizeof(Vertex);
	unsigned int offset = 0;

	// Set the vertex buffer to active in the input assembler so it can be rendered
	deviceContext->IASetVertexBuffers(0, 1, &vertBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered
	deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the 2D primitive type used to create the boxecule; we're making one boxecule,
	// so triangular primitives make the most sense :P
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
}

void AthruRect::SetMaterial(Material& freshMaterial)
{
	material = freshMaterial;
}

Material& AthruRect::GetMaterial()
{
	return material;
}

SQT& AthruRect::FetchTransformations()
{
	return transformations;
}

DirectX::XMMATRIX AthruRect::GetTransform()
{
	return transformations.asMatrix();
}

// Push constructions for this class through Athru's custom allocator
void* AthruRect::operator new(size_t size)
{
	StackAllocator* allocator = ServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<AthruRect>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void AthruRect::operator delete(void* target)
{
	return;
}