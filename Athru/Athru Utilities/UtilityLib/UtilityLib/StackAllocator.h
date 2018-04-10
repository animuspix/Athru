#pragma once

#include <cstdlib>
#include "AthruGlobals.h"

class ServiceCentre;

// Manages the heap with a single-ended stack data structure
// Not to be confused with the program's internal stack
class StackAllocator
{
	struct Marker
	{
		eightByteUnsigned distanceFromTop;
	};

	public:
		StackAllocator(const eightByteUnsigned& expectedMemoryUsage);
		~StackAllocator();

		// Allocate [bytes] from the available heap memory
		address AlignedAlloc(eightByteUnsigned bytes,
							 byteUnsigned alignment,
							 bool setMarker);

		// Allocate a single non-aligned byte from the available heap memory
		address ByteAlloc(bool setMarker);

		// Clear memory down to a particular address; all pointers
		// up to and including the given address should be emptied
		// for simplicity and to prevent undesirable overwrites
		// from occurring
		void DeAlloc(MemoryStuff::MARKER_INDEX_TYPE markerIndex);

		// Retrieve the top-most address from the stack
		address GetTop();

		// Retrieve the number of active markers
		twoByteUnsigned GetActiveMarkerCount();

	private:
		// Variables
		address stackTop;
		address stackStart;
		Marker markers[MemoryStuff::MAX_MARKER_COUNT];
		twoByteUnsigned activeMarkerCount;
		eightByteUnsigned availMem;

		// Helper functions
		// Shift pointers to a given alignment
		address PtrAdjuster(address srcPtr,
							byteUnsigned alignment);
};