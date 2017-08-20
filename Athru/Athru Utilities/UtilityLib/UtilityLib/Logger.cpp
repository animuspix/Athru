#include "Logger.h"
#include "UtilityServiceCentre.h"

// The console-printer's output string is statically declared, so
// define it here
char Logger::ConsolePrinter::outputString[8191];

// Push constructions for this class through Athru's custom allocator
void* Logger::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<Logger>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void Logger::operator delete(void* target)
{
	return;
}