#include "Chunk.h"
#include "Boxecule.h"

Chunk::Chunk(byteSigned offsetFromHomeX, byteSigned offsetFromHomeZ)
{
	// Calls to procedural-generation subsystems would happen here,
	// but I should be focussing on rendering stuffs for now; the
	// chunk will just contain a plane instead :P
	chunkBoxecules = new Boxecule[CHUNK_WIDTH][CHUNK_WIDTH][CHUNK_WIDTH];
	for (byteUnsigned i = 0; i < CHUNK_WIDTH; i += 1)
	{
		for (byteUnsigned j = 0; j < CHUNK_WIDTH; j += 1)
		{
			for (byteUnsigned k = 0; k < CHUNK_WIDTH; k += 1)
			{
				(*chunkBoxecules)[i][j][k] = Boxecule(0.7f, 0.5f, 0.5f, 0.8f);
				DirectX::XMVECTOR boxeculePos = _mm_set_ps(i + offsetFromHomeX, j, k + offsetFromHomeZ, 1);
				DirectX::XMVECTOR boxeculeRot = _mm_set_ps(0, 0, 0, 0);
				chunkBoxecules[i][j][k].FetchTransformations() = SQT(boxeculeRot, boxeculePos, 1);
				flattenedChunkBoxecules[0] = &chunkBoxecules[i][j][k];
			}
		}
	}
}

Chunk::~Chunk()
{
}

void Chunk::Update(DirectX::XMVECTOR playerPosition)
{
	// Weather updates (weather would really be calculated per-planet,
	// but some processing would still happen in the chunks closest to
	// the player), organism updates, and terrain generation + disk
	// storage (if terrain is never passed to disk then it won't
	// persist between plays) will happen here in the future;
	// sticking to a simple lit + shadowed bumpy plane for now :)
}

Boxecule** Chunk::GetChunkBoxecules()
{
	return flattenedChunkBoxecules;
}
