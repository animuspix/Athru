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

	// Update the previous/current sytems
	lastSys = currSys;
	currSys = galaxy->GetCurrentSystem(mainCamera->GetTranslation());

	// Non-GPU updates here (networking, player stats, UI stuffs, etc.)
}

SceneFigure::Figure* Scene::CollectLocalFigures()
{
	// Pass the player's local environment into an array of
	// generic [SceneFigure]s
	Planet** planets = currSys->GetPlanets();
	currFigures[0] = currSys->GetStar()->GetCoreFigure();
	currFigures[1] = planets[0]->GetCoreFigure();
	currFigures[2] = planets[1]->GetCoreFigure();
	currFigures[3] = planets[2]->GetCoreFigure();
	currFigures[4] = planets[3]->GetCoreFigure();
	currFigures[5] = planets[4]->GetCoreFigure();
	currFigures[6] = planets[5]->GetCoreFigure();
	currFigures[7] = planets[6]->GetCoreFigure();
	currFigures[8] = planets[7]->GetCoreFigure();
	currFigures[9] = planets[8]->GetCoreFigure();
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

bool Scene::CheckFreshSys()
{
	return (currSys = lastSys);
}

// Push constructions for this class through Athru's custom allocator
void* Scene::operator new(size_t size)
{
	StackAllocator* allocator = AthruCore::Utility::AccessMemory();
	return allocator->AlignedAlloc(size, (uByte)std::alignment_of<Scene>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void Scene::operator delete(void* target)
{
	return;
}
