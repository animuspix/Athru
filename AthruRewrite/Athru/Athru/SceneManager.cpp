#include "ServiceCentre.h"
#include "SceneManager.h"

SceneManager::SceneManager()
{
	boxeculeSet = DEBUG_NEW Boxecule*[MAX_NUM_STORED_BOXECULES];

	nearChunks[0] = new Chunk(-1 * CHUNK_WIDTH, CHUNK_WIDTH);
	nearChunks[1] = new Chunk(0, CHUNK_WIDTH);
	nearChunks[2] = new Chunk(CHUNK_WIDTH, CHUNK_WIDTH);
	nearChunks[3] = new Chunk(-1 * CHUNK_WIDTH, 0);
	nearChunks[4] = new Chunk(0, 0);
	nearChunks[5] = new Chunk(CHUNK_WIDTH, 0);
	nearChunks[6] = new Chunk(-1 * CHUNK_WIDTH, -1 * CHUNK_WIDTH);
	nearChunks[7] = new Chunk(0, -1 * CHUNK_WIDTH);
	nearChunks[8] = new Chunk(CHUNK_WIDTH, -1 * CHUNK_WIDTH);
	
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
	nearChunks[0]->Update(playerPosition);
	nearChunks[1]->Update(playerPosition);
	nearChunks[2]->Update(playerPosition);
	nearChunks[3]->Update(playerPosition);
	nearChunks[4]->Update(playerPosition);
	nearChunks[5]->Update(playerPosition);
	nearChunks[6]->Update(playerPosition);
	nearChunks[7]->Update(playerPosition);
	nearChunks[8]->Update(playerPosition);

	// Copy chunk-boxecules across into the array of scene boxecules
	Boxecule** nearChunk0Boxecules = nearChunks[0]->GetChunkBoxecules();
	Boxecule** nearChunk1Boxecules = nearChunks[1]->GetChunkBoxecules();
	Boxecule** nearChunk2Boxecules = nearChunks[2]->GetChunkBoxecules();
	Boxecule** nearChunk3Boxecules = nearChunks[3]->GetChunkBoxecules();
	Boxecule** homeChunkBoxecules = nearChunks[4]->GetChunkBoxecules();
	Boxecule** nearChunk5Boxecules = nearChunks[5]->GetChunkBoxecules();
	Boxecule** nearChunk6Boxecules = nearChunks[6]->GetChunkBoxecules();
	Boxecule** nearChunk7Boxecules = nearChunks[7]->GetChunkBoxecules();
	Boxecule** nearChunk8Boxecules = nearChunks[8]->GetChunkBoxecules();

	Boxecule** nearChunkBoxecules[8] = { nearChunk0Boxecules,
										 nearChunk1Boxecules,
										 nearChunk2Boxecules,
										 nearChunk3Boxecules,
										 nearChunk5Boxecules,
										 nearChunk6Boxecules,
										 nearChunk7Boxecules,
										 nearChunk8Boxecules };

	// Copy boxecules from non-home chunks
	eightByteUnsigned homeChunkOffset = 0;
	byteUnsigned chunkIndex = 0;
	for (eightByteUnsigned i = 0; i < (8 * CHUNK_VOLUME); i += CHUNK_VOLUME)
	{
		for (eightByteUnsigned j = 0; j < CHUNK_VOLUME; j += 1)
		{
			boxeculeSet[i + j] = nearChunkBoxecules[chunkIndex][j];
		}

		chunkIndex += 1;
		homeChunkOffset = (i + CHUNK_VOLUME);
	}

	// Copy boxecules from the home chunk
	for (eightByteUnsigned i = 0; i < CHUNK_VOLUME * boxeculeDensity; i += 1)
	{
		boxeculeSet[i + homeChunkOffset] = homeChunkBoxecules[i];
	}
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