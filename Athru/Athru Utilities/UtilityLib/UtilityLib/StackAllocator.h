#pragma once

#include <cstdlib>
#include "AppGlobals.h"

class ServiceCentre;

// Manages the heap with a single-ended stack data structure
// Not to be confused with the program's internal stack
class StackAllocator
{
	struct Marker
	{
		u8Byte distanceFromTop;
	};

	public:
		StackAllocator(const u8Byte& expectedMemoryUsage);
		~StackAllocator();

		// Allocate [bytes] from the available heap memory
		address AlignedAlloc(u8Byte bytes,
							 uByte alignment,
							 bool setMarker);

		// Allocate a single non-aligned byte from the available heap memory
		address ByteAlloc(bool setMarker);

		// Clear memory down to a particular address; all pointers
		// up to and including the given address should be emptied
		// for simplicity and to prevent undesirable overwrites
		// from occurring
		void DeAlloc(MemoryStuff::MARKER_INDEX_TYPE markerIndex);

		// Retrieve the top-most address from the stack
		// All allocations related to [stackTop] should go through [AlignedAlloc(...)]
		// or [ByteAlloc(...)], so this returns immutably
		const address& GetTop();

		// Retrieve the global heap offset for the Athru memory stack
		// All allocations become untraceable if [start] is changed after
		// the iniital [malloc], so this returns an immutable reference
		// (similarly to [GetTop()])
		const address& GetStart();

		// Retrieve the number of active markers
		u2Byte GetActiveMarkerCount();

	private:
		// Variables
		address stackTop;
		address stackStart;
		Marker markers[MemoryStuff::MAX_MARKER_COUNT];
		u2Byte activeMarkerCount;
		u8Byte availMem;

		// Helper functions
		// Shift pointers to a given alignment
		address PtrAdjuster(address srcPtr,
							uByte alignment);
};