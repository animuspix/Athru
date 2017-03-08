#include "Logger.h"
#include "ServiceCentre.h"

// Push constructions for this class through Athru's custom allocator
void* Logger::operator new(size_t size)
{
	StackAllocator* allocator = ServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<Logger>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void Logger::operator delete(void* target)
{
	return;
}