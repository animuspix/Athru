#include "HiLevelServiceCentre.h"
#include <cstdlib>
#include "Camera.h"
#include "Boxecule.h"
#include "Critter.h"
#include "Chunk.h"

Chunk::Chunk(byteSigned offsetFromHomeX, byteSigned offsetFromHomeZ, Critter& testCritter, byteUnsigned chunkIndex)
{
	chunkPoints[0] = _mm_set_ps(0, (float)(offsetFromHomeZ), 0, (float)offsetFromHomeX);
	chunkPoints[1] = _mm_set_ps(0, (float)(offsetFromHomeZ - CHUNK_WIDTH), 0, (float)offsetFromHomeX);
	chunkPoints[2] = _mm_set_ps(0, (float)(offsetFromHomeZ), 0, (float)(offsetFromHomeX + CHUNK_WIDTH));
	chunkPoints[3] = _mm_set_ps(0, (float)(offsetFromHomeZ - CHUNK_WIDTH), 0, (float)(offsetFromHomeX + CHUNK_WIDTH));

	subChunks = DEBUG_NEW SubChunk*[SUB_CHUNK_VOLUME];

	for (byteUnsigned i = 0; i < SUB_CHUNKS_PER_CHUNK; i += 1)
	{
		if (chunkIndex == testCritter.GetTorsoTransformations().pos.m128_f32[3] &&
			i == testCritter.GetTorsoTransformations().pos.m128_f32[2])
		{
			subChunks[i] = new SubChunk(this, i, offsetFromHomeX, offsetFromHomeZ, &testCritter);
		}

		else
		{
			subChunks[i] = new SubChunk(this, i, offsetFromHomeX, offsetFromHomeZ, nullptr);
		}
	}
}

Chunk::~Chunk()
{
	for (byteUnsigned i = 0; i < SUB_CHUNKS_PER_CHUNK; i += 1)
	{
		subChunks[i]->~SubChunk();
	}

	delete subChunks;
	subChunks = nullptr;
}

void Chunk::Update(DirectX::XMVECTOR& playerPosition)
{
	// Weather updates (weather would really be calculated per-planet,
	// but some processing would still happen in the chunks closest to
	// the player), organism updates, and terrain generation + disk
	// storage (if terrain is never passed to disk then it won't
	// persist between plays) will happen here in the future;
	// sticking to a simple lit + shadowed bumpy plane for now :)

	// Realllllllllly hacky way to keep the demo directional light
	// within the FOV so it can't be culled; this needs to be replaced
	// with a more permanent solution (e.g. nothing moves at all,
	// terrain + lighting is streamed to the player's "position" on the
	// planet) as soon as possible
	Boxecule* directionalLightBoxecule = subChunks[SUB_CHUNKS_PER_CHUNK - 1]->GetStoredBoxecules()[CHUNK_WIDTH / 2];
	subChunks[SUB_CHUNKS_PER_CHUNK - 1]->GetStoredBoxecules()[CHUNK_WIDTH / 2]->FetchTransformations().pos = _mm_add_ps(playerPosition, _mm_set_ps(0, 1, 3, 1));
}

DirectX::XMVECTOR* Chunk::GetChunkPoints()
{
	return chunkPoints;
}

SubChunk** Chunk::GetSubChunks()
{
	return subChunks;
}

// Push constructions for this class through Athru's custom allocator
void* Chunk::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<Chunk>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void Chunk::operator delete(void* target)
{
	return;
}