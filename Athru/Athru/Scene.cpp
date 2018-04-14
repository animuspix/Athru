#include "UtilityServiceCentre.h"
#include "GPUServiceCentre.h"
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
	// Update the camera
	mainCamera->Update();

	// Non-GPU updates here (networking, player stats, UI stuffs, etc.)
}

SceneFigure* Scene::CollectLocalFigures()
{
	// Pass the player's local environment into an array of
	// generic [SceneFigure]s
	System* currSys = galaxy->GetCurrentSystem(mainCamera->GetTranslation());
	Star* star = currSys->GetStar();
	Planet** planets = currSys->GetPlanets();
	currFigures[0] = *(SceneFigure*)(star);
	currFigures[1] = *(SceneFigure*)(planets[0]);
	currFigures[2] = *(SceneFigure*)(planets[1]);
	currFigures[3] = *(SceneFigure*)(planets[2]);
	currFigures[4] = *(SceneFigure*)(planets[3]);
	currFigures[5] = *(SceneFigure*)(planets[4]);
	currFigures[6] = *(SceneFigure*)(planets[5]);
	currFigures[7] = *(SceneFigure*)(planets[6]);
	currFigures[8] = *(SceneFigure*)(planets[7]);
	currFigures[9] = *(SceneFigure*)(planets[8]);
	return currFigures;
}

Camera* Scene::GetMainCamera()
{
	return mainCamera;
}

Galaxy* Scene::GetGalaxy()
{
	return galaxy;
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
