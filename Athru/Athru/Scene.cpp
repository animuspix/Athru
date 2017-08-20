#include "UtilityServiceCentre.h"
#include "Scene.h"

Scene::Scene()
{
	mainCamera = new Camera();
	galaxy = new Galaxy(AVAILABLE_GALACTIC_LAYOUTS::SPHERE);
	gpuSceneCourier = new GPUSceneCourier();
}

Scene::~Scene()
{
	// Free any un-managed memory associated
	// with the main camera
	mainCamera->~Camera();

	// Send the reference associated with the
	// main camera to [nullptr]
	mainCamera = nullptr;

	// Free any un-managed memory associated
	// with the GPU messenger object
	gpuSceneCourier->~GPUSceneCourier();

	// Send the reference associated with the
	// GPU messenger object to [nullptr]
	gpuSceneCourier = nullptr;
}

void Scene::Update()
{
	// Update the camera
	mainCamera->Update();

	// Update the galaxy
	galaxy->Update();

	// Pass each of the generated figures in the system along
	// to the GPU for rendering
	SceneFigure currFigures[SceneStuff::MAX_NUM_SCENE_FIGURES];
	//currFigures[0] = *(SceneFigure*)(GetCurrentSystem().GetStar());
	currFigures[0] = *(SceneFigure*)(GetCurrentSystem().GetPlanets()[0]);
	gpuSceneCourier->CommitSceneToGPU(currFigures, 1);
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

GPUSceneCourier* Scene::GetGPUCourier()
{
	return gpuSceneCourier;
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
