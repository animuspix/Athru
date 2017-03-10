#pragma once

#include <directxmath.h>
#include "Typedefs.h"
#include "Boxecule.h"
#include "Chunk.h"

#define MAX_CHUNKS_PER_METER 100
#define MIN_NUM_STORED_BOXECULES CHUNK_VOLUME * 9
#define MAX_NUM_STORED_BOXECULES (8 * CHUNK_VOLUME) + (CHUNK_VOLUME * MAX_CHUNKS_PER_METER)
#define BOXECULE_MEMORY_ALLOCATION MAX_NUM_STORED_BOXECULES * sizeof(Boxecule)

class Boxecule;
class SceneManager
{
	public:
		SceneManager();
		~SceneManager();

		Boxecule** GetSceneBoxecules();
		fourByteUnsigned CurrBoxeculeCount();
		void Update(DirectX::XMVECTOR playerPosition);

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void * target);

	private:
		// Chunks containing the raw boxecules around the player
		// Drawn order (where P is the chunk surrounding the player) is
		// [0, 1, 2]
		// [3, 4P, 5]
		// [6, 7, 8]
		Chunk* nearChunks[9];

		// HUGE pointer containing sub-pointers to all the boxecules that will
		// ever exist in the scene at once
		// Only the subset defined by multiplying the minimum number of boxecules
		// by the current boxecule density is actually passed to the
		// render manager each frame
		Boxecule** boxeculeSet;

		// Boxecule scale as a proportion of the default boxecule size
		// ([1])
		float boxeculeScale;

		// Current boxecule density in terms of boxecules/spatial unit
		twoByteUnsigned boxeculeDensity;
};

