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
	sceneChunks[0].Update(playerPosition);
	sceneChunks[1].Update(playerPosition);
	sceneChunks[2].Update(playerPosition);
	sceneChunks[3].Update(playerPosition);
	sceneChunks[4].Update(playerPosition);
	sceneChunks[5].Update(playerPosition);
	sceneChunks[6].Update(playerPosition);
	sceneChunks[7].Update(playerPosition);
	sceneChunks[8].Update(playerPosition);
	sceneChunks[9].Update(playerPosition);
}
