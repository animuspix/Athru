#include "UtilityServiceCentre.h"
#include "Scene.h"

Scene::Scene()
{
	mainCamera = new Camera();
	galaxy = new Galaxy(AVAILABLE_GALACTIC_LAYOUTS::SPHERE);
}

Scene::~Scene()
{
	// Free any un-managed memory associated
	// with the main camera
	mainCamera->~Camera();

	// Send the reference associated with the
	// main camera to [nullptr]
	mainCamera = nullptr;
}

void Scene::Update()
{
	mainCamera->Update();
	galaxy->Update();
}

Camera* Scene::GetMainCamera()
{
	return mainCamera;
}

Galaxy* Scene::GetGalaxy()
{
	return galaxy;
}

System& Scene::GetCurrentSystem()
{
	return galaxy->GetCurrentSystem(mainCamera->GetTranslation());
}

// Push constructions for this class through Athru's custom allocator
void* Scene::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<Scene>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void Scene::operator delete(void* target)
{
	return;
}
