#include "ServiceCentre.h"
#include "Critter.h"
#include "SceneManager.h"

SceneManager::SceneManager()
{
	Critter localTestCritter;

	nearChunks[0] = new Chunk(-1 * CHUNK_WIDTH, CHUNK_WIDTH, localTestCritter, 0);
	nearChunks[1] = new Chunk(0, CHUNK_WIDTH, localTestCritter, 1);
	nearChunks[2] = new Chunk(CHUNK_WIDTH, CHUNK_WIDTH, localTestCritter, 2);
	nearChunks[3] = new Chunk(-1 * CHUNK_WIDTH, 0, localTestCritter, 3);
	nearChunks[4] = new Chunk(0, 0, localTestCritter, 4);
	nearChunks[5] = new Chunk(CHUNK_WIDTH, 0, localTestCritter, 5);
	nearChunks[6] = new Chunk(-1 * CHUNK_WIDTH, -1 * CHUNK_WIDTH, localTestCritter, 6);
	nearChunks[7] = new Chunk(0, -1 * CHUNK_WIDTH, localTestCritter, 7);
	nearChunks[8] = new Chunk(CHUNK_WIDTH, -1 * CHUNK_WIDTH, localTestCritter, 8);

	boxeculeDensity = 1;
}

SceneManager::~SceneManager()
{
	nearChunks[0]->~Chunk();
	nearChunks[1]->~Chunk();
	nearChunks[2]->~Chunk();
	nearChunks[3]->~Chunk();
	nearChunks[4]->~Chunk();
	nearChunks[5]->~Chunk();
	nearChunks[6]->~Chunk();
	nearChunks[7]->~Chunk();
	nearChunks[8]->~Chunk();
}

Chunk** SceneManager::GetChunks()
{
	return nearChunks;
}

twoByteUnsigned SceneManager::GetBoxeculeDensity()
{
	return boxeculeDensity;
}

fourByteUnsigned SceneManager::GetBoxeculeCount()
{
	return boxeculeDensity * MIN_NUM_STORED_BOXECULES;
}

void SceneManager::Update(DirectX::XMVECTOR playerPosition)
{
	// Update each chunk within the scene
	// HUGE threading opportunity here - implement once current
	// to-do is cleared :)
	nearChunks[0]->Update(playerPosition);
	nearChunks[1]->Update(playerPosition);
	nearChunks[2]->Update(playerPosition);
	nearChunks[3]->Update(playerPosition);
	nearChunks[4]->Update(playerPosition);
	nearChunks[5]->Update(playerPosition);
	nearChunks[6]->Update(playerPosition);
	nearChunks[7]->Update(playerPosition);
	nearChunks[8]->Update(playerPosition);
}

// Push constructions for this class through Athru's custom allocator
void* SceneManager::operator new(size_t size)
{
	StackAllocator* allocator = ServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<SceneManager>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void SceneManager::operator delete(void* target)
{
	return;
}