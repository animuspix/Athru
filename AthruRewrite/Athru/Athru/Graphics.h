#pragma once

#include "Direct3D.h"
#include "AthruGlobals.h"

class RenderManager;
class SceneManager;
class Camera;
class Graphics
{
	public:
		Graphics(HWND hwnd);
		// Initialiser, needed to avoid a dependency loop between
		// [this] and RenderManager (required in the main game loop,
		// but dependant on [this])
		void FetchDependencies();
		~Graphics();

		void Update();
		void Draw();
		Direct3D* GetD3D();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// Connection to the DirectX graphics processing backend
		Direct3D* d3D;

		// Connection to the player's camera
		Camera* camera;

		// The below don't /have/ to go in here, and prob's shouldn't;
		// they're in this class because that's where the game loop
		// is atm :P

		// Pointer to the render manager
		RenderManager* renderManagerPttr;

		// Pointer to the scene manager
		SceneManager* sceneManagerPttr;
};

