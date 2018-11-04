#include "UtilityServiceCentre.h"
#include "Critter.h"
#include "Planet.h"

Planet::Planet(float givenScale,
			   DirectX::XMFLOAT3 position, DirectX::XMVECTOR qtnRotation,
			   DirectX::XMVECTOR* distCoeffs) :
		SceneFigure(position, givenScale,
					distCoeffs)
{
	// Initialise plants
	plants = new SceneFigure[SceneStuff::MAX_NUM_SCENE_ORGANISMS];

	// Generation logic undefined for now...

	// Initialise critters
	// Seven days to create and validate procedural animals isn't really enough time...restrict them to spheres/cubes
	// for now
	critters = new SceneFigure[SceneStuff::MAX_NUM_SCENE_ORGANISMS];

	// Generation logic undefined for now...
}

Planet::~Planet() {}

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
