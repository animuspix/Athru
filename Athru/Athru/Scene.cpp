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
	System currSys = GetCurrentSystem();
	Planet* currPlanet = currSys.GetPlanets()[0];
	currFigures[0] = *(SceneFigure*)(currSys.GetStar());
	currFigures[1] = *(SceneFigure*)(currPlanet);
	currFigures[2] = currPlanet->FetchCritter(0);
	currFigures[3] = currPlanet->FetchCritter(1);
	currFigures[4] = currPlanet->FetchCritter(2);
	currFigures[5] = currPlanet->FetchCritter(3);
	currFigures[6] = currPlanet->FetchPlant(0);
	currFigures[7] = currPlanet->FetchPlant(1);
	currFigures[8] = currPlanet->FetchPlant(2);
	currFigures[9] = currPlanet->FetchPlant(3);
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
