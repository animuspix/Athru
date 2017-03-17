#pragma once

#include <directxmath.h>
#include "Typedefs.h"

#define CHUNK_WIDTH 15
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
		DirectX::XMVECTOR* GetChunkPoints();

		void* operator new(size_t size);
		void operator delete(void * target);

	private:
		// Raw array of boxecules within this chunk
		Boxecule** chunkBoxecules;

		// 2D points defining the area covered by this chunk
		DirectX::XMVECTOR chunkPoints[4];

		// Where [this] is on it's associated planet
		unsigned int planetaryIndex;
};

