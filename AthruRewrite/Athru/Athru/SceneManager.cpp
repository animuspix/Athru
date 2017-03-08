#include "ServiceCentre.h"
#include "SceneManager.h"

SceneManager::SceneManager()
{
	StackAllocator* stackAllocatorPttr = ServiceCentre::AccessMemory();
	ServiceCentre::AccessLogger()->Log(MAX_NUM_STORED_BOXECULES, Logger::DESTINATIONS::CONSOLE);
	boxeculeSet = new Boxecule*[MAX_NUM_STORED_BOXECULES];

	nearChunks[0] = Chunk(-1 * CHUNK_WIDTH, CHUNK_WIDTH);
	nearChunks[1] = Chunk(0, CHUNK_WIDTH);
	nearChunks[2] = Chunk(CHUNK_WIDTH, CHUNK_WIDTH);
	nearChunks[3] = Chunk(-1 * CHUNK_WIDTH, 0);
	nearChunks[4] = Chunk(0, 0);
	nearChunks[5] = Chunk(CHUNK_WIDTH, 0);
	nearChunks[6] = Chunk(-1 * CHUNK_WIDTH, -1 * CHUNK_WIDTH);
	nearChunks[7] = Chunk(0, -1 * CHUNK_WIDTH);
	nearChunks[8] = Chunk(CHUNK_WIDTH, -1 * CHUNK_WIDTH);
}

SceneManager::~SceneManager()
{
}

Boxecule** SceneManager::GetSceneBoxecules()
{
	return boxeculeSet;
}

fourByteUnsigned SceneManager::CurrBoxeculeCount()
{
	return boxeculeDensity * MIN_NUM_STORED_BOXECULES;
}

void SceneManager::Update(DirectX::XMVECTOR playerPosition)
{
	// Update each chunk within the scene
	// HUGE threading opportunity here - implement once current
	// to-do is cleared :)
	nearChunks[0].Update(playerPosition);
	nearChunks[1].Update(playerPosition);
	nearChunks[2].Update(playerPosition);
	nearChunks[3].Update(playerPosition);
	nearChunks[4].Update(playerPosition);
	nearChunks[5].Update(playerPosition);
	nearChunks[6].Update(playerPosition);
	nearChunks[7].Update(playerPosition);
	nearChunks[8].Update(playerPosition);

	// Copy chunk-boxecules across into the array of scene boxecules
	boxeculeSet[0] = *(nearChunks[0].GetChunkBoxecules());
	boxeculeSet[1] = *(nearChunks[1].GetChunkBoxecules());
	boxeculeSet[2] = *(nearChunks[2].GetChunkBoxecules());
	boxeculeSet[3] = *(nearChunks[3].GetChunkBoxecules());
	boxeculeSet[4] = *(nearChunks[4].GetChunkBoxecules());
	boxeculeSet[5] = *(nearChunks[5].GetChunkBoxecules());
	boxeculeSet[6] = *(nearChunks[6].GetChunkBoxecules());
	boxeculeSet[7] = *(nearChunks[7].GetChunkBoxecules());
	boxeculeSet[8] = *(nearChunks[8].GetChunkBoxecules());
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