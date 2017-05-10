#include "stdafx.h"
#include "SceneVertex.h"

// Push constructions for this struct through Athru's custom allocator
void* SceneVertex::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<SceneVertex>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void SceneVertex::operator delete(void* target)
{
	return;
}