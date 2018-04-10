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

	AthruUtilities::UtilityServiceCentre::AccessLogger()->Log("Logging arithmetic types \n",
															  Logger::DESTINATIONS::CONSOLE);
	AthruUtilities::UtilityServiceCentre::AccessLogger()->Log('a',
															  Logger::DESTINATIONS::CONSOLE);
	AthruUtilities::UtilityServiceCentre::AccessLogger()->Log((short)SHRT_MAX,
															  Logger::DESTINATIONS::CONSOLE);
	AthruUtilities::UtilityServiceCentre::AccessLogger()->Log(INT_MAX,
															  Logger::DESTINATIONS::CONSOLE);
	AthruUtilities::UtilityServiceCentre::AccessLogger()->Log(LLONG_MAX,
															  Logger::DESTINATIONS::CONSOLE);
	AthruUtilities::UtilityServiceCentre::AccessLogger()->Log(56.0625f,
															  Logger::DESTINATIONS::CONSOLE);
	AthruUtilities::UtilityServiceCentre::AccessLogger()->Log(56.0625,
															  Logger::DESTINATIONS::CONSOLE);
	AthruUtilities::UtilityServiceCentre::AccessLogger()->Log(56.0625l,
															  Logger::DESTINATIONS::CONSOLE);
	AthruUtilities::UtilityServiceCentre::AccessLogger()->Log(false,
															  Logger::DESTINATIONS::CONSOLE);

	class testClass { };
	testClass tstClass;
	struct testStruct { };
	testStruct tstStruct;
	union testUnion { };
	testUnion tstUnion;
	AthruUtilities::UtilityServiceCentre::AccessLogger()->Log("Logging structs/classes, unions, and enums\n",
															  Logger::DESTINATIONS::CONSOLE);
	AthruUtilities::UtilityServiceCentre::AccessLogger()->Log(tstStruct,
															  Logger::DESTINATIONS::CONSOLE);
	AthruUtilities::UtilityServiceCentre::AccessLogger()->Log(tstClass,
															  Logger::DESTINATIONS::CONSOLE);
	AthruUtilities::UtilityServiceCentre::AccessLogger()->Log(tstUnion,
															  Logger::DESTINATIONS::CONSOLE);
	AthruUtilities::UtilityServiceCentre::AccessLogger()->Log(FIG_TYPES::PLANET,
															  Logger::DESTINATIONS::CONSOLE);

	fourByteSigned intArr[6] = { 0, 1, -2, 4, -3, 6 };
	AthruUtilities::UtilityServiceCentre::AccessLogger()->Log("Logging global functions, member functions, and example pointer/array data\n",
															  Logger::DESTINATIONS::CONSOLE);
	AthruUtilities::UtilityServiceCentre::AccessLogger()->Log(TimeStuff::FPS,
															  Logger::DESTINATIONS::CONSOLE);
	AthruUtilities::UtilityServiceCentre::AccessLogger()->Log(&Scene::CollectLocalFigures,
															  Logger::DESTINATIONS::CONSOLE);
	AthruUtilities::UtilityServiceCentre::AccessLogger()->Log(this,
															  Logger::DESTINATIONS::CONSOLE);
	AthruUtilities::UtilityServiceCentre::AccessLogger()->LogArray(intArr,
																   sizeof(intArr) / sizeof(fourByteUnsigned),
																   Logger::DESTINATIONS::CONSOLE);

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
