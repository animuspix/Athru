#include "ServiceCentre.h"
#include "Boxecule.h"

Boxecule::Boxecule(float r, float g, float b, float a)
{
	// Long integer used to represent success/failure for different DirectX operations
	HRESULT result;

	// Initialise the material associated with [this]
	material = Material(Sound(),
						r, g, b, a,
						AVAILABLE_SHADERS::RASTERIZER,
						AVAILABLE_SHADERS::NULL_SHADER,
						AVAILABLE_SHADERS::NULL_SHADER,
						AVAILABLE_SHADERS::NULL_SHADER,
						AVAILABLE_SHADERS::NULL_SHADER);

	// Cache the color associated with [this]
	float* color = material.GetColorData();

	// Initialise vertices
	// X and Y coordinates look a helluva lot like magic numbers; they were basically
	// chosen out of trial and error while I was looking for points that'd line up
	// into a reasonable cube on a 16:9 screen without too much distortion
	Vertex vertices[8] = { Vertex(-0.5f, 0.5f, -0.5f,
								   color), // Front plane, upper left (v0)

						   Vertex(0.5f, 0.5f, -0.5f,
								  color), // Front plane, upper right (v1)

						   Vertex(-0.5f, -0.5f, -0.5f,
								   color), // Front plane, lower left (v2)

						   Vertex(0.5f, -0.5f, -0.5f,
								  color), // Front plane, lower right (v3)

						   Vertex(0.5f, -0.5f, 0.5f,
								  color), // Back plane, lower right (v4)

						   Vertex(0.5f, 0.5f, 0.5f,
								  color), // Back plane, upper right (v5)

						   Vertex(-0.5f, 0.5f, 0.5f,
								   color), // Back plane, upper left (v6)

						   Vertex(-0.5f, -0.5f, 0.5f,
								   color) }; // Back plane, lower left (v7)

	// Initialise indices
	// Each set of three values is one triangle
	long indices[36] = { 0,
						 1,
						 2,

						 2,
						 1,
						 3,

						 2,
						 3,
						 4,

						 4,
						 3,
						 1,

						 4,
						 1,
						 5,

						 5,
						 1,
						 0,

						 5,
						 0,
						 6,

						 6,
						 0,
						 2,

						 6,
						 2,
						 7,

						 2,
						 4,
						 7,

						 7,
						 4,
						 6,

						 4,
						 5,
						 6 };


	// Set up the description of the static vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(Vertex) * 8;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Place a reference to the vertices in a subresource structure
	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Create the vertex buffer
	ID3D11Device* device = ServiceCentre::AccessGraphics()->GetD3D()->GetDevice();
	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);

	// Set up the description of the static index buffer
	D3D11_BUFFER_DESC indexBufferDesc;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * 36;
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
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);

	// Check if anything failed during the geometry setup
	assert(SUCCEEDED(result));

	// Initialise the scale, rotation, and position of [this]
	transformations = SQT();
}

Boxecule::~Boxecule()
{
	// Release the index buffer.
	m_indexBuffer->Release();
	m_indexBuffer = 0;

	// Release the vertex buffer.
	m_vertexBuffer->Release();
	m_vertexBuffer = 0;
}

void Boxecule::PassToGPU(ID3D11DeviceContext* deviceContext)
{
	// Set vertex buffer stride and offset.
	unsigned int stride = sizeof(Vertex);
	unsigned int offset = 0;

	// Set the vertex buffer to active in the input assembler so it can be rendered
	deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered
	deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the 2D primitive type used to create the boxecule; we're making one boxecule,
	// so triangular primitives make the most sense :P
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
}

void Boxecule::SetMaterial(Material& freshMaterial)
{
	material = freshMaterial;
}

Material Boxecule::GetMaterial()
{
	return material;
}

SQT& Boxecule::FetchTransformations()
{
	return transformations;
}

DirectX::XMMATRIX Boxecule::GetTransform()
{
	return transformations.asMatrix();
}

// Push constructions for this class through Athru's custom allocator
void* Boxecule::operator new(size_t size)
{
	StackAllocator* allocator = ServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<Boxecule>(), false);
}

// Push constructions for this class through Athru's custom allocator
void* Boxecule::operator new[](size_t size)
{
	StackAllocator* allocator = ServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<Boxecule>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void Boxecule::operator delete(void* target)
{
	return;
}

// We aren't expecting to use [delete[]], so overload it to do nothing;
void Boxecule::operator delete[](void* target)
{
	return;
}
