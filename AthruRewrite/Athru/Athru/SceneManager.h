#pragma once

#include <directxmath.h>
#include "Typedefs.h"
#include "Chunk.h"

#define MIN_NUM_BOXECULES 9000000
#define MAX_NUM_BOXECULES MIN_NUM_BOXECULES * 1000

class Boxecule;
class SceneManager
{
	public:
		SceneManager();
		~SceneManager();

		Boxecule** GetSceneBoxecules();
		fourByteUnsigned CurrBoxeculeCount();
		void Update(DirectX::XMVECTOR playerPosition);

	private:
		// Chunks containing the raw boxecules in the
		// scene
		// Drawn order (where P is the chunk surrounding the player) is 
		// [0, 1, 2]
		// [3, 4P, 5]
		// [6, 7, 8]
		Chunk sceneChunks[9];

		// HUGE pointer containing sub-pointers to all the boxecules that will
		// ever exist in the scene at once
		Boxecule** boxeculeSet;

		// Slightly less huge pointer containing sub-pointers to all the boxecules
		// that currently exist in the scene

		// Boxecule scale as a proportion of the default boxecule size
		// ([1])
		float boxeculeScale;

		// Boxecule density in terms of boxecules/spatial unit
		twoByteUnsigned boxeculeDensity;
};

