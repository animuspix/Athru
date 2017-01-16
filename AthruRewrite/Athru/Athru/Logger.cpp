#include "Logger.h"
#include "ServiceCentre.h"

// Push constructions for this class through Athru's custom allocator
void* Logger::operator new(size_t size)
{
	StackAllocator* allocator = ServiceCentre::Instance().AccessMemory();
	return allocator->AlignedAlloc((fourByteUnsigned)size, 4, false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void Logger::operator delete(void* target)
{
	return;
}