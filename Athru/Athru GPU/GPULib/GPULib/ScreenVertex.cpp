#include "UtilityServiceCentre.h"
#include "PostVertex.h"

// Push constructions for this struct through Athru's custom allocator
void* PostVertex::operator new(size_t size)
{
	StackAllocator* allocator = AthruCore::Utility::AccessMemory();
	return allocator->AlignedAlloc(size, (uByte)std::alignment_of<PostVertex>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void PostVertex::operator delete(void* target)
{
	return;
}
