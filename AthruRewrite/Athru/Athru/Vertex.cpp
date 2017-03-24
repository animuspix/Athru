#include "ServiceCentre.h"
#include "Vertex.h"

void* Vertex::operator new(size_t size)
{
	StackAllocator* allocator = ServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<Vertex>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void Vertex::operator delete(void* target)
{
	return;
}