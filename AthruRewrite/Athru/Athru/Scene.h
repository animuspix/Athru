#pragma once

#include "Camera.h"

class Scene
{
	public:
		Scene();
		~Scene();
		void Update();

		// Retrieve a reference to the
		// main camera associated with [this]
		Camera* GetMainCamera();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		Camera* mainCamera;
};

