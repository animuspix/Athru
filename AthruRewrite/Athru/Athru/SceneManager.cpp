#include "SceneManager.h"

SceneManager::SceneManager()
{
}

SceneManager::~SceneManager()
{
}

Boxecule** SceneManager::GetSceneBoxecules()
{
	return nullptr;
}

fourByteUnsigned SceneManager::CurrBoxeculeCount()
{
	return boxeculeDensity * MIN_NUM_BOXECULES;
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
	nearChunks[9].Update(playerPosition);
}
