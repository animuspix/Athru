#include "UtilityServiceCentre.h"
#include "PostVertex.h"

// Push constructions for this struct through Athru's custom allocator
void* PostVertex::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<PostVertex>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void PostVertex::operator delete(void* target)
{
	return;
}
