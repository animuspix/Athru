#pragma once

#include <directxmath.h>

class Boxecule;
class Chunk
{
	public:
		Chunk();
		~Chunk();

		void Update(DirectX::XMVECTOR playerPosition);
		Boxecule** GetChunkBoxecules();

	private:
		Boxecule* 
};

