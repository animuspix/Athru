#pragma once

#include "GPUScene.h"
#include "Camera.h"
#include "Galaxy.h"

class Scene
{
	public:
		Scene();
		~Scene();
		void Update();

		// Retrieve a reference to the
		// main camera associated with [this]
		Camera* GetMainCamera();

		// Retrieve a reference to the galaxy associated
		// with [this]
		Galaxy* GetGalaxy();

		// Retrieve the system closest to the given camera
		// position
		System& GetCurrentSystem();

		// Retrieve a reference to the Scene/GPU and GPU/Scene messenger
		// class
		GPUScene& FetchGPUMessenger();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		GPUScene gpuSceneMessenger;
		Galaxy* galaxy;
		Camera* mainCamera;
};

