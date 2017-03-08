#pragma once

#include <directxmath.h>
#include "Typedefs.h"

#define CHUNK_WIDTH 35
#define CHUNK_VOLUME CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_WIDTH

class Boxecule;
class Chunk
{
	public:
		Chunk() {}
		Chunk(byteSigned offsetFromHomeX, byteSigned offsetFromHomeZ);
		~Chunk();

		void Update(DirectX::XMVECTOR playerPosition);
		Boxecule** GetChunkBoxecules();

	private:
		// Raw 3D array of boxecules within this chunk
		Boxecule*** chunkBoxecules;

		// "Flattened" 1D array of pointers to the boxecules within
		// this chunk
		Boxecule* flattenedChunkBoxecules[CHUNK_WIDTH];

		// Where [this] is on it's associated planet
		unsigned int planetaryIndex;
};

