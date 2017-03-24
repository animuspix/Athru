#include "ServiceCentre.h"
#include "Mesh.h"

Mesh::Mesh(Material meshMaterial)
{
	material = meshMaterial;
}

Mesh::~Mesh()
{
	// Release the index buffer
	indexBuffer->Release();
	indexBuffer = 0;

	// Release the vertex buffer
	vertBuffer->Release();
	vertBuffer = 0;
}

void Mesh::PassToGPU(ID3D11DeviceContext* deviceContext)
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

void Mesh::SetMaterial(Material& freshMaterial)
{
	material = freshMaterial;
}

Material& Mesh::GetMaterial()
{
	return material;
}

SQT& Mesh::FetchTransformations()
{
	return transformations;
}

Vertex* Mesh::GetVerts()
{
	return vertices;
}

DirectX::XMMATRIX Mesh::GetTransform()
{
	return transformations.asMatrix();
}

// Push constructions for this class through Athru's custom allocator
void* Mesh::operator new(size_t size)
{
	StackAllocator* allocator = ServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<Mesh>(), false);
}

// Push constructions for this class through Athru's custom allocator
void* Mesh::operator new[](size_t size)
{
	StackAllocator* allocator = ServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<Mesh>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void Mesh::operator delete(void* target)
{
	return;
}

// We aren't expecting to use [delete[]], so overload it to do nothing
void Mesh::operator delete[](void* target)
{
	return;
}