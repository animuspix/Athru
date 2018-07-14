#include <assert.h>
#include "StackAllocator.h"

StackAllocator::StackAllocator(const eightByteUnsigned& expectedMemoryUsage)
{
	stackStart = malloc(expectedMemoryUsage); // Perform initial allocation
	stackTop = stackStart; // Initialize the stack-offset to zero (no internal allocations have occurred)
	availMem = expectedMemoryUsage; // No internal allocations, so the available-memory tracker can safely
									// initialize to [expectedMemoryUsage]

	// No allocations, so no active in-memory markers; record that here
	// (inactive markers are kept at [stackStart])
	for (MemoryStuff::MARKER_INDEX_TYPE i = 0; i < MemoryStuff::MAX_MARKER_COUNT; i += 1)
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

address StackAllocator::PtrAdjuster(address srcPtr,
									byteUnsigned alignment)
{
	// "Memory alignment" describes a modular series where addresses occur every [alignment] bytes
	// in memory; this means we can find the amount [srcPtr] is offset from the most-recent aligned
	// byte by taking it into that series with [(addrValType)srcPtr % alignmentPadded]
	// We can't move pointers "backwards" into occupied memory, though, so our overall adjustment
	// becomes the distance between [srcPtr] and the /next/ aligned byte, which we can compute
	// with [alignmentPadded - [(addrValType)srcPtr % alignmentPadded]]; this works because
	// sequences of natural numbers are complementary, so e.g. a number four values into a base-10
	// series is necessarily six values away from [0] and the start/end of the sequence
	MemoryStuff::addrValType alignmentPadded = (MemoryStuff::addrValType)alignment;
	byteUnsigned memAdjustment = (byteUnsigned)(alignmentPadded - ((MemoryStuff::addrValType)srcPtr % alignmentPadded));

	// Align the memory, then store it in a single-byte pointer so we can easily keep the amount of
	// adjustment used nearby
	byteUnsigned* alignedMemory = (byteUnsigned*)srcPtr + memAdjustment;

	// Record the adjustment applied to the memory in the byte before the memory itself
	alignedMemory[-1] = memAdjustment;

	// Return the adjusted memory
	return alignedMemory;
}

address StackAllocator::AlignedAlloc(eightByteUnsigned bytes,
									 byteUnsigned alignment,
									 bool setMarker)
{
	// Only accept power-of-two alignments
	assert((alignment % 0x2) == 0);

	// Generate a pointer inside the allocated stack at the
	// appropriate offset
	address rawMemory;
	address returnableMemory;
	eightByteUnsigned allocSize = bytes + alignment;
	assert(((eightByteSigned)availMem - (eightByteSigned)allocSize) > 0); // Immediately flag memory-accesses beyond the depth of the stack
	if (!setMarker)
	{
		rawMemory = stackTop;
		returnableMemory = PtrAdjuster(rawMemory, alignment);
		stackTop = (byteUnsigned*)returnableMemory + allocSize + 1;
		for (MemoryStuff::MARKER_INDEX_TYPE i = 0; i < (activeMarkerCount + 1); i += 1)
		{
			markers[i].distanceFromTop += (eightByteUnsigned)(((byteUnsigned*)returnableMemory - (byteUnsigned*)rawMemory) + allocSize + 1);
		}
	}
	else
	{
		rawMemory = (byteUnsigned*)stackTop + 1;
		returnableMemory = PtrAdjuster(rawMemory, alignment);
		stackTop = (byteUnsigned*)returnableMemory + allocSize + 2;
		for (MemoryStuff::MARKER_INDEX_TYPE i = 0; i < (activeMarkerCount + 1); i += 1)
		{
			markers[i].distanceFromTop += (eightByteUnsigned)(((byteUnsigned*)returnableMemory - (byteUnsigned*)rawMemory) + allocSize + 2);
		}
		activeMarkerCount += 1;
	}
	availMem -= allocSize;
	return returnableMemory;
}

address StackAllocator::ByteAlloc(bool setMarker)
{
	assert(((eightByteSigned)availMem - (eightByteSigned)1) > 0); // Immediately flag memory-accesses beyond the depth of the stack
	address returnableMemory;
	if (!setMarker)
	{
		returnableMemory = stackTop;
		byteUnsigned* prevByte = (byteUnsigned*)returnableMemory - 1;
		*prevByte = 0; // No alignment for single-byte allocations, so zero the previous byte
		stackTop = (byteUnsigned*)stackTop + 2;
		for (MemoryStuff::MARKER_INDEX_TYPE i = 0; i < (activeMarkerCount + 1); i += 1)
		{
			markers[i].distanceFromTop += 2;
		}
	}
	else
	{
		returnableMemory = (byteUnsigned*)stackTop + 1;
		byteUnsigned* prevByte = (byteUnsigned*)returnableMemory - 1;
		*prevByte = 0; // No alignment for single-byte allocations, so zero the previous byte
		stackTop = (byteUnsigned*)stackTop + 3;
		for (MemoryStuff::MARKER_INDEX_TYPE i = 0; i < (activeMarkerCount + 1); i += 1)
		{
			markers[i].distanceFromTop += 3;
		}
		activeMarkerCount += 1;
	}
	availMem -= 1;
	return returnableMemory;
}

void StackAllocator::DeAlloc(MemoryStuff::MARKER_INDEX_TYPE markerIndex)
{
	// Check that the de-allocation won't empty the whole stack
	assert(markers[markerIndex].distanceFromTop > 0);

	// Zero the de-allocated memory
	memset((byteUnsigned*)stackTop - markers[markerIndex].distanceFromTop, NULL, markers[markerIndex].distanceFromTop);

	// Update the [top] of the memory-stack
	stackTop = (byteUnsigned*)stackTop - (markers[markerIndex].distanceFromTop);

	// Update available memory
	availMem += markers[markerIndex].distanceFromTop;

	// Update markers appropriately
	for (MemoryStuff::MARKER_INDEX_TYPE i = (activeMarkerCount + 1); i > markerIndex; i -= 1)
	{
		markers[i - 1].distanceFromTop = 0;
		activeMarkerCount = i - 1;
	}
}

address StackAllocator::GetTop()
{
	return stackTop;
}

twoByteUnsigned StackAllocator::GetActiveMarkerCount()
{
	return activeMarkerCount;
}