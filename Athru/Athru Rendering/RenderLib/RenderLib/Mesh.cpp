#include "UtilityServiceCentre.h"
#include "Mesh.h"

Mesh::Mesh()
{
	// Constructions handled in child classes
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

// Very unsure about alignments here; ask around at AIE when possible

// Push constructions for this class through Athru's custom allocator
void* Mesh::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<Mesh>(), false);
}

// Push constructions for this class through Athru's custom allocator
void* Mesh::operator new[](size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
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
