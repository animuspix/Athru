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
	// No real need to have this loop here, replace it with deeper logic when
	// possible
	SceneFigure currFigures[SceneStuff::MAX_NUM_SCENE_FIGURES];
	for (fourByteUnsigned i = 0; i < SceneStuff::MAX_NUM_SCENE_FIGURES; i += 1)
	{
		currFigures[i] = SceneFigure();
	}

	// Pass the player's local environment into an array of
	// generic [SceneFigure]s
	currFigures[0] = *(SceneFigure*)(GetCurrentSystem().GetPlanets()[0]);
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
