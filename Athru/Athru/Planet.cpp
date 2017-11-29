#include "UtilityServiceCentre.h"
#include "Critter.h"
#include "Planet.h"

Planet::Planet(float givenMass, float givenRadius, DirectX::XMVECTOR givenAvgColor,
			   DirectX::XMVECTOR offsetFromStar, DirectX::XMFLOAT3 rotation) :
		SceneFigure(_mm_set_ps(0, 0, 0, 0), _mm_set_ps(0, 0, 0, 0), _mm_set_ps(1, 0, 0, 0),
					_mm_set_ps(1, 0, 0, 0), givenRadius, givenAvgColor, FIG_TYPES::PLANET)
{
	// Initialise plants
	plants = new SceneFigure[SceneStuff::MAX_NUM_SCENE_ORGANISMS];
	for (fourByteUnsigned i = 0; i < SceneStuff::MAX_NUM_SCENE_ORGANISMS; i += 1)
	{
		plants[i] = SceneFigure();
	}

	// Initialise critters
	critters = new SceneFigure[SceneStuff::MAX_NUM_SCENE_ORGANISMS];
	for (fourByteUnsigned i = 0; i < SceneStuff::MAX_NUM_SCENE_ORGANISMS; i += 1)
	{
		critters[i] = SceneFigure();
	}
}

Planet::~Planet()
{
}

SceneFigure& Planet::FetchCritter(fourByteUnsigned ndx)
{
	return critters[ndx];
}

SceneFigure& Planet::FetchPlant(fourByteUnsigned ndx)
{
	return plants[ndx];
}

// Push constructions for this class through Athru's custom allocator
void* Planet::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<Planet>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void Planet::operator delete(void* target)
{
	return;
}
