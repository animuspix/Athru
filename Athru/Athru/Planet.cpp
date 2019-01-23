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
	plants = new SceneFigure[SceneStuff::PLANTS_PER_PLANET];

	// Generation logic undefined for now...

	// Initialise critters
	// Seven days to create and validate procedural animals isn't really enough time...restrict them to spheres/cubes
	// for now
	critters = new SceneFigure[SceneStuff::ANIMALS_PER_PLANET];

	// Generation logic undefined for now...
}

Planet::~Planet() {}

SceneFigure& Planet::FetchCritter(u4Byte ndx)
{
	return critters[ndx];
}

SceneFigure& Planet::FetchPlant(u4Byte ndx)
{
	return plants[ndx];
}

// Push constructions for this class through Athru's custom allocator
void* Planet::operator new(size_t size)
{
	StackAllocator* allocator = AthruCore::Utility::AccessMemory();
	return allocator->AlignedAlloc(size, (uByte)std::alignment_of<Planet>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void Planet::operator delete(void* target)
{
	return;
}
