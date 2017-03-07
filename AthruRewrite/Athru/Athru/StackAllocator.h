#pragma once

#include <cstdlib>
#include "Typedefs.h"
#include "SceneManager.h"

#define STARTING_HEAP (eightByteUnsigned)(BOXECULE_MEMORY_ALLOCATION * 1.4)
#define MAX_MARKER_COUNT 120
#define MARKER_INDEX_TYPE byteSigned
#define POINTER_LENGTH eightByteUnsigned

class ServiceCentre;

// Manages the heap with a single-ended stack data structure
// Not to be confused with the program's internal stack
class StackAllocator
{
	struct Marker
	{
		fourByteUnsigned distanceFromTop;
	};

	public:
		StackAllocator();
		~StackAllocator();

		// Allocate [bytes] from the available heap memory
		address AlignedAlloc(fourByteUnsigned bytes, byteUnsigned alignment, bool setMarker);

		// Allocate a single non-aligned byte from the available heap memory
		address ByteAlloc(bool setMarker);

		// Clear memory down to a particular address; all pointers
		// up to and including the given address will be emptied
		// for simplicity and to prevent undesirable overwrites
		// from occurring
		void DeAlloc(MARKER_INDEX_TYPE markerIndex);

		// Set a clearance marker without allocating additional memory
		void SetMarker();

		// Retrieve the top-most address from the stack
		address GetTop();

		// Retrieve the number of active markers
		twoByteUnsigned GetActiveMarkerCount();

	private:
		// Variables
		address stackTop;
		address stackStart;
		Marker markers[MAX_MARKER_COUNT];
		twoByteUnsigned activeMarkerCount;

		// Helper functions
		// Shift pointers to a given alignment
		address PtrAdjuster(address srcPtr, byteUnsigned alignment);
};