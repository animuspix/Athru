#pragma once

#include "Boxecule.h"
#include "Critter.h"

class Chunk;
class SubChunk
{
	public:
		SubChunk() {}
		SubChunk(Chunk* parent,
				 float index, float parentOffsetX, float parentOffsetZ,
				 Critter* testCritter);
		~SubChunk();

		Boxecule** GetStoredBoxecules();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void * target);

	private:
		// Reference to the super-chunk associated
		// with [this]
		Chunk* superChunk;

		// Points used to define the ZY bounds of [this] so
		// it can be easily tested during calls to
		// [GetVisibility(...)]
		DirectX::XMVECTOR subChunkPoints[4];

		// Raw array of boxecules within this sub-chunk
		Boxecule** storedBoxecules;

};

