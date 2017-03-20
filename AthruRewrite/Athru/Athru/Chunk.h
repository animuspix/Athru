#pragma once

#include <directxmath.h>
#include "Typedefs.h"
#include "SubChunk.h"

#define CHUNK_WIDTH 16
#define CHUNK_VOLUME CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_WIDTH

#define SUB_CHUNK_DEPTH 2
#define SUB_CHUNKS_PER_CHUNK (CHUNK_WIDTH / SUB_CHUNK_DEPTH)
#define SUB_CHUNK_VOLUME CHUNK_VOLUME / SUB_CHUNKS_PER_CHUNK

class Boxecule;
class Chunk
{
	public:
		Chunk() {}
		Chunk(byteSigned offsetFromHomeX, byteSigned offsetFromHomeZ);
		~Chunk();

		void Update(DirectX::XMVECTOR& playerPosition);
		bool GetVisibility(Camera* mainCamera);
		DirectX::XMVECTOR* GetChunkPoints();
		SubChunk** GetSubChunks();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void * target);

	private:
		// Sub-chunks within [this]
		SubChunk** subChunks;

		// 2D points defining the area covered by this chunk
		DirectX::XMVECTOR chunkPoints[4];

		// Where [this] is on it's associated planet
		unsigned int planetaryIndex;
};

