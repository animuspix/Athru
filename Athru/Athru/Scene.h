#pragma once

#include "Camera.h"
#include "Galaxy.h"

class Scene
{
	public:
		Scene();
		~Scene();
		void Update();
		SceneFigure* CollectLocalFigures();

		// Retrieve a reference to the
		// main camera associated with [this]
		Camera* GetMainCamera();

		// Retrieve a reference to the galaxy associated
		// with [this]
		Galaxy* GetGalaxy();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// Reference to the galaxy containing the game
		Galaxy* galaxy;

		// Reference to the player camera
		Camera* mainCamera;

		// Transfer array used to move data out of the "scene" and into the GPU for
		// highly-parallel updates/rendering
		SceneFigure currFigures[SceneStuff::MAX_NUM_SCENE_FIGURES];
};
