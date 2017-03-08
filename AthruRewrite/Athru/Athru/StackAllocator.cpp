#include <assert.h>
#include "ServiceCentre.h"
#include "StackAllocator.h"

StackAllocator::StackAllocator()
{
	stackStart = malloc(STARTING_HEAP);
	stackTop = stackStart;

	for (MARKER_INDEX_TYPE i = 0; i < MAX_MARKER_COUNT; i += 1)
	{
		markers[i].distanceFromTop = 0;
	}
	activeMarkerCount = 0;
}

StackAllocator::~StackAllocator()
{
	stackTop = stackStart;
	free(stackTop);
	stackTop = nullptr;
}

address StackAllocator::PtrAdjuster(address srcPtr, byteUnsigned alignment)
{
	byteUnsigned ptrMask = alignment - 1;
	byteUnsigned memoryMisalignment = (POINTER_LENGTH)srcPtr & (POINTER_LENGTH)ptrMask;
	byteUnsigned memoryAdjustment = alignment - memoryMisalignment;

	// Align the memory, then store it in a char* so we can easily keep the amount of
	// adjustment used nearby
	char* alignedMemory = (char*)srcPtr + memoryAdjustment;

	// Record the adjustment applied to the memory in the byte before the memory itself
	alignedMemory[-1] = memoryAdjustment;

	// Return the adjusted memory
	return (char*)srcPtr + memoryAdjustment;
}

address StackAllocator::AlignedAlloc(eightByteUnsigned bytes, byteUnsigned alignment, bool setMarker)
{
	// Only accept power-of-two alignments
	assert((alignment & (alignment - 1)) == 0);

	address rawMemory;
	address returnableMemory;
	eightByteUnsigned allocSize = bytes + alignment;
	if (!setMarker)
	{
		rawMemory = stackTop;
		returnableMemory = PtrAdjuster(rawMemory, alignment);
		stackTop = (char*)returnableMemory + allocSize + 1;
		for (MARKER_INDEX_TYPE i = 0; i < (activeMarkerCount + 1); i += 1)
		{
			markers[i].distanceFromTop += (eightByteUnsigned)(((char*)returnableMemory - (char*)rawMemory) + allocSize + 1);
		}
	}
	else
	{
		rawMemory = (char*)stackTop + 1;
		returnableMemory = PtrAdjuster(rawMemory, alignment);
		stackTop = (char*)returnableMemory + allocSize + 2;
		for (MARKER_INDEX_TYPE i = 0; i < (activeMarkerCount + 1); i += 1)
		{
			markers[i].distanceFromTop += (eightByteUnsigned)(((char*)returnableMemory - (char*)rawMemory) + allocSize + 2);
		}

		activeMarkerCount += 1;
	}
	return returnableMemory;
}

address StackAllocator::ByteAlloc(bool setMarker)
{
	address returnableMemory;
	if (!setMarker)
	{
		returnableMemory = stackTop;
		stackTop = (char*)stackTop + 2;
		for (MARKER_INDEX_TYPE i = 0; i < (activeMarkerCount + 1); i += 1)
		{
			markers[i].distanceFromTop += 2;
		}
	}
	else
	{
		returnableMemory = (char*)stackTop + 1;
		stackTop = (char*)stackTop + 3;
		for (MARKER_INDEX_TYPE i = 0; i < (activeMarkerCount + 1); i += 1)
		{
			markers[i].distanceFromTop += 3;
		}
		activeMarkerCount += 1;
	}
	return returnableMemory;
}

void StackAllocator::DeAlloc(MARKER_INDEX_TYPE markerIndex)
{
	assert(markers[markerIndex].distanceFromTop > 0);
	stackTop = (char*)stackTop - (activeMarkerCount - markerIndex);

	for (MARKER_INDEX_TYPE i = (activeMarkerCount + 1); i > markerIndex; i -= 1)
	{
		markers[i - 1].distanceFromTop = 0;
		activeMarkerCount = i - 1;
	}
}

void StackAllocator::SetMarker()
{
	stackTop = (char*)stackTop + 2;

	for (MARKER_INDEX_TYPE i = 0; i < (activeMarkerCount + 1); i += 1)
	{
		markers[i].distanceFromTop += 2;
	}
	activeMarkerCount += 1;
}

address StackAllocator::GetTop()
{
	return stackTop;
}

twoByteUnsigned StackAllocator::GetActiveMarkerCount()
{
	return activeMarkerCount;
}