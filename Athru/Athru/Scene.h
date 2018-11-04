#pragma once

#include "Camera.h"
#include "Galaxy.h"

class Scene
{
	public:
		Scene();
		~Scene();
		void Update();
		SceneFigure::Figure* CollectLocalFigures();

		// Retrieve a reference to the
		// main camera associated with [this]
		Camera* GetMainCamera();

		// Retrieve a reference to the galaxy associated
		// with [this]
		Galaxy* GetGalaxy();

		// Check whether the player moved between systems in the last frame
		bool CheckFreshSys();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// Reference to the galaxy containing the game
		Galaxy* galaxy;

		// Current player system
		System* currSys;

		// Player system in the previous frame
		System* lastSys;

		// Reference to the player camera
		Camera* mainCamera;

		// Transfer array used to move data out of the "scene" and into the GPU for
		// highly-parallel updates/rendering
		SceneFigure::Figure currFigures[SceneStuff::MAX_NUM_SCENE_FIGURES];
};

