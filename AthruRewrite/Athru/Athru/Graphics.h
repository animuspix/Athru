#pragma once

#include "Direct3D.h"
#include "MathIncludes.h"

#define FULL_SCREEN false
#define VSYNC_ENABLED false
#define DISPLAY_WIDTH 1024
#define DISPLAY_HEIGHT 768
#define DISPLAY_ASPECT_RATIO DISPLAY_WIDTH / DISPLAY_HEIGHT
#define SCREEN_FAR 1000.0f
#define SCREEN_NEAR 0.1f
#define VERT_FIELD_OF_VIEW_RADS PI / 4

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

		void Frame();
		Direct3D* GetD3D();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		void Render();

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

