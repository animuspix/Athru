#pragma once

#include <directxmath.h>

#define CHUNK_WIDTH 50
#define CHUNK_VOLUME CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_WIDTH

class Boxecule;
class Chunk
{
	public:
		Chunk();
		~Chunk();

		void Update(DirectX::XMVECTOR playerPosition);
		Boxecule*** GetChunkBoxecules();

	private:
		Boxecule*** chunkBoxecules;
		unsigned int planetaryIndex;
};

