#include "UtilityServiceCentre.h"
#include "GPURand.h"

// Push constructions for this class through Athru's custom allocator
void* GPURand::operator new(size_t size)
{
	StackAllocator* allocator = AthruCore::Utility::AccessMemory();
	return allocator->AlignedAlloc(size, (uByte)std::alignment_of<GPURand>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void GPURand::operator delete(void* target)
{
	return;
}