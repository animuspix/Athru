#pragma once

#include <directxmath.h>
#include "Typedefs.h"
#include "Boxecule.h"
#include "Chunk.h"
#include "Planet.h"
#include "DirectionalLight.h"

#define MAX_CHUNKS_PER_METER 1000
#define MIN_NUM_STORED_BOXECULES CHUNK_VOLUME * 9
#define MAX_NUM_STORED_BOXECULES (8 * CHUNK_VOLUME) + (CHUNK_VOLUME * 1000)
#define BOXECULE_MEMORY_ALLOCATION MAX_NUM_STORED_BOXECULES * sizeof(Boxecule)

enum class SYSTEM_LOCATIONS
{
	ORIGIN,
	ALPHA,
	BETA,
	GAMMA,
	DELTA,
	EPSILON,
	ZETA,
	ETA,
	THETA,
	IOTA,
	KAPPA,
	OFF_PLANET
};

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
		// The current location of the player in the system
		SYSTEM_LOCATIONS playerLocation;

		// The star associated with this system
		DirectionalLight sun;

		// Array of planets in the system
		Planet planets[10];
		
		// Chunks containing the raw boxecules around the player
		// Drawn order (where P is the chunk surrounding the player) is 
		// [0, 1, 2]
		// [3, 4P, 5]
		// [6, 7, 8]
		Chunk nearChunks[9];

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

