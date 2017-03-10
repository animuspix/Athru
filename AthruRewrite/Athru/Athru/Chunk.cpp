#include "MathIncludes.h"
#include "ServiceCentre.h"
#include "Boxecule.h"
#include <cstdlib>
#include "Chunk.h"

Chunk::Chunk(byteSigned offsetFromHomeX, byteSigned offsetFromHomeZ)
{
	// Calls to procedural-generation subsystems would happen here,
	// but I should be focussing on rendering stuffs for now; the
	// chunk will just contain a plane instead :P
	chunkBoxecules = DEBUG_NEW Boxecule*[CHUNK_VOLUME];

	for (eightByteSigned i = 0; i < CHUNK_VOLUME; i += 1)
	{
		chunkBoxecules[i] = new Boxecule(0.7f, 0.5f, 0.5f, 0.8f);
		DirectX::XMVECTOR boxeculePos = _mm_set_ps(1, ((i / CHUNK_WIDTH) % CHUNK_WIDTH) + offsetFromHomeZ, i % CHUNK_WIDTH, i / (CHUNK_WIDTH * CHUNK_WIDTH) + offsetFromHomeX);
		DirectX::XMVECTOR boxeculeRot = DirectX::XMQuaternionRotationRollPitchYaw(0, 0, 0);
		chunkBoxecules[i]->FetchTransformations() = SQT(boxeculeRot, boxeculePos, 1);
 	}
}

Chunk::~Chunk()
{
	for (eightByteUnsigned i = 0; i < CHUNK_VOLUME; i += 1)
	{
		chunkBoxecules[i]->~Boxecule();
	}

	delete chunkBoxecules;
	chunkBoxecules = nullptr;
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
	return chunkBoxecules;
}

// Push constructions for this class through Athru's custom allocator
void* Chunk::operator new(size_t size)
{
	StackAllocator* allocator = ServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<Chunk>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void Chunk::operator delete(void* target)
{
	return;
}