#include "UtilityServiceCentre.h"
#include "Critter.h"
#include "Planet.h"

Planet::Planet(float givenMass, float givenRadius, DirectX::XMVECTOR givenAvgColor,
			   DirectX::XMVECTOR offsetFromStar, DirectX::XMVECTOR qtnRotation,
			   DirectX::XMVECTOR* distCoeffs, DirectX::XMVECTOR* rgbaCoeffs) :
		SceneFigure(_mm_set_ps(0, 0, 0, 0), _mm_set_ps(0, 0, 0, 0), _mm_set_ps(1, 0, 0, 0),
					_mm_set_ps(1, 0, 0, 0), givenRadius, (fourByteUnsigned)FIG_TYPES::PLANET, distCoeffs, rgbaCoeffs,
					1)
{
	// Initialise plants
	plants = new SceneFigure[SceneStuff::MAX_NUM_SCENE_ORGANISMS];
	for (fourByteUnsigned i = 0; i < SceneStuff::MAX_NUM_SCENE_ORGANISMS; i += 1)
	{
		// Surface properties
		// (= branch placement/growth functions, PDE constants)
		DirectX::XMVECTOR distCoeffs[10];

		// Should replace [rand()] with a better RNG at some point...

		// First vector defines plant format (domed/coniferous), branch placement
		// (Golden-ratio orbits or even spacings), pre-branch trunk height, and
		// tree age (used to calculate branch counts + tree size)
		distCoeffs[0] = _mm_set_ps(float(rand() % PlantStuff::MAX_PLANT_AGE),
								   float(1.0 / (rand() % 100)),
								   float(rand() % 2),
								   float(rand() % 2));

		// Second vector defines plant branch envelope (embedded in two 3D angles),
		// maximum proportional branch length, and branch count/fork
		distCoeffs[1] = _mm_set_ps(PlantStuff::MAX_BRANCH_COUNT_AT_FORK / (rand() % PlantStuff::MAX_BRANCH_COUNT_AT_FORK),
								   1.0f / (rand() % 4),
								   360.0f / float(rand()),
								   360.0f / float(rand()));

		// Third vector defines growth properties (forking rate, change in branch length) (embedded in the sigma coefficient of a
		// log-normal distribution), maximum possible size, tropic frequency (embedded in a multiplier over an ordinary sin wave),
		// and plant type (woody/vascular)
		distCoeffs[2] = _mm_set_ps(1.0f / (rand() % 4),
								   max(PlantStuff::MAX_PLANT_SIZE / (rand() % 10), 1.0f),
								   PlantStuff::MAX_TROPISM / (rand() % 96),
								   rand() % 2);

		// Fourth vector defines stem/branch rigidity/planarity (relevant to vascular plants, treated as 1.0 and 0 for woody plants),
		// whether or not the current plant is branch-bearing (relevant to vascular plants, all trees are assumed to carry branches),
		// baseline leaves-per-branch/stem (relevant to vascular plants, all trees have one leaf/branch)
		//distCoeffs[3]

		// Fifth-through-seventh vectors define leaf lobe counts + venation properties (research needed here) (skippable for now)

		// Eighth-through-tenth vectors define textural properties (esp. bark diffusion) (research needed here) (skippable for now)

		// Core research areas:
		// - Fast venation
		// - Bark diffusion
		// - Cylinder generation + placement + gravitropism (heavy branches will displace tissue at their base and curve towards the
		//   ground as they travel further away from their parent branch/trunk)
		// - Leaf generation + placement
		// - Natural bark coloration, chlorophyll diffusion
		// - Planetary tree placement

		// Bark/vascular coloration properties
		DirectX::XMVECTOR rgbaCoeffs[10];
		plants[i] = SceneFigure(_mm_set_ps(1, 0, 0, 0),
								_mm_set_ps(0, givenRadius, givenRadius, givenRadius), // Assume planets are spherical for now...
								_mm_set_ps(1, 0, 0, 0),
								_mm_set_ps(1, 0, 0, 0),
								1.0,
								(fourByteUnsigned)FIG_TYPES::CRITTER,
								distCoeffs,
								rgbaCoeffs,
								1);
	}

	// Initialise critters
	// Seven days to create and validate procedural animals isn't really enough time...restrict them to spheres/cubes
	// for now
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
