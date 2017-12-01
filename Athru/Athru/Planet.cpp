#include "UtilityServiceCentre.h"
#include "Critter.h"
#include "Planet.h"

Planet::Planet(float givenMass, float givenRadius,
			   DirectX::XMVECTOR offsetFromStar, DirectX::XMVECTOR qtnRotation,
			   DirectX::XMVECTOR* distCoeffs, DirectX::XMVECTOR* rgbaCoeffs) :
		SceneFigure(_mm_set_ps(0, 0, 0, 0), _mm_set_ps(0, 0, 0, 0), _mm_set_ps(1, 0, 0, 0),
					_mm_set_ps(1, 0, 0, 0), givenRadius, (fourByteUnsigned)FIG_TYPES::PLANET,
					distCoeffs, rgbaCoeffs,
					1)
{
	// Initialise plants
	plants = new SceneFigure[SceneStuff::MAX_NUM_SCENE_ORGANISMS];
	for (fourByteUnsigned i = 0; i < SceneStuff::MAX_NUM_SCENE_ORGANISMS; i += 1)
	{
		// Surface properties
		// (= branch placement/growth functions, PDE constants)
		DirectX::XMVECTOR plantDistCoeffs[10];

		// Should replace [rand()] with a better RNG at some point...

		// First vector defines plant format (domed/coniferous), branch placement
		// (Golden-ratio orbits or even spacings), pre-branch trunk height, and
		// tree age (used to calculate branch counts + tree size)
		plantDistCoeffs[0] = _mm_set_ps((float)(rand() % PlantStuff::MAX_PLANT_AGE),
								   (float)(1.0 / ((rand() % 100) + 1)),
								   (float)(rand() % 2),
								   (float)(rand() % 2));

		// Second vector defines plant branch envelope (embedded in two 3D angles),
		// maximum proportional branch length, and branch count/fork
		plantDistCoeffs[1] = _mm_set_ps((float)(PlantStuff::MAX_BRANCH_COUNT_AT_FORK / (rand() % PlantStuff::MAX_BRANCH_COUNT_AT_FORK)),
								   1.0f / ((rand() % 4) + 1),
								   360.0f / (float)(rand()),
								   360.0f / (float)(rand()));

		// Third vector defines growth properties (forking rate, change in branch length) (embedded in the sigma coefficient of a
		// log-normal distribution), maximum possible size, tropic frequency (embedded in a multiplier over an ordinary sin wave),
		// and plant type (woody/vascular)
		plantDistCoeffs[2] = _mm_set_ps(1.0f / (rand() % 4),
								   max(PlantStuff::MAX_PLANT_SIZE / ((rand() % 10) + 1), 1.0f),
								   PlantStuff::MAX_TROPISM / ((rand() % 96) + 1),
								   (float)(rand() % 2));

		// Fourth vector defines stem/branch rigidity (relevant to vascular plants, treated as 1.0 for woody plants),
		// whether or not the current plant is branch-bearing (relevant to vascular plants, all trees are assumed to carry branches),
		// whether this is a single-leaf or multi-leaf plant, and leaf lobe counts
		plantDistCoeffs[3] = _mm_set_ps(1.0f / ((rand() % 10) + 1),
								   (float)(rand() % 2),
								   (float)(rand() % 2),
								   (float)(rand() % PlantStuff::MAX_LEAF_LOBE_COUNT));

		// Fifth vector defines leaf vein colour
		// Hardcoded soft blue veins for now (might introduce extra variation in future)
		// Alpha channel treated as leaf translucency, faintly randomized
		plantDistCoeffs[4] = _mm_set_ps(0.5f, 0.67f, 0.84f, abs((1.0f / rand()) - 0.5f));

		// Sixth vector defines lobe shape (via cubic bezier offsets in x and y) and roughness (via frequency/amplitude
		// values components of a saw wave)
		plantDistCoeffs[5] = _mm_set_ps(1.0f / (rand() % 10),
								   1.0f / (rand() % 10),
								   1.0f / (rand() % 10),
								   1.0f / (rand() % 10));

		// Seventh-through-ninth vectors define textural properties (esp. bark diffusion) (research needed here) (skippable for now)
		// Bark SHOULD be organised through a cylindrical diffusion equation...
		// But I have no training in PDEs at all and probably wouldn't get anything done in the (very, very) tired state
		// I'm in atm
		// So just lazy sine layering for now
		// Every first value gives a frequency, second values give amplitudes, third values give weights
		// Fourth values give cutoff height (for rounder/flatter sheets of bark)
		// Sines are projected parallel to the edges of each cylindrical branch/stem/trunk
		plantDistCoeffs[6] = _mm_set_ps(4.0f,
								   0.1f,
								   0.75f,
								   0.075f);

		plantDistCoeffs[7] = _mm_set_ps(8.0f,
								   0.05f,
								   0.125f,
								   0.025f);

		plantDistCoeffs[8] = _mm_set_ps(2.0f,
								   0.05f,
								   0.125f,
								   0.05f);

		// Core research areas:
		// - Fast venation
		// - Bark diffusion
		// - Cylinder generation + placement + gravitropism (heavy branches will displace tissue at their base and curve towards the
		//   ground as they travel further away from their parent branch/trunk)
		// - Leaf generation + placement
		// - Natural bark coloration, chlorophyll diffusion
		// - Planetary tree placement
		// Venation model adopted from "Modelling and visualization of leaf venation patterns", published by the
		// University of Calgary via the Association for Computing Machinery (ACM)
		// Vein growth is assumed to be reticulate in all cases (venation is ignored for grass stems)

		// Populate venation buffer here (carries ~16384 points split into auxin sources + vein nodes)
		// (full growth occurs during population)
		//for ()
		//{
		//
		//}

		// Auxin sources render as chlorophyll, vein nodes render as some vein color (see above)
		// Leaf color function finds the closest venation node and returns the associated color;
		// search is optimized by reorganising the cells into a flat grid and only searching within
		// the grid cell containing the area of intersection (i.e. a very basic quadtree optimization)

		// Venation theory is simpler than expected, still unsure how to efficiently manipulate plant
		// trunks/branches within the plant SDF or whether any of my theory will actually work; should
		// create a test build in C# (for leaf generation) + a small shader (for simplified tree
		// generation) and see where those end up...

		// Bark/vascular coloration properties
		DirectX::XMVECTOR plantRGBACoeffs[10];

		// First vector describes baseline tissue colour
		plantRGBACoeffs[0] = _mm_set_ps(0.94f, 1.0f, 1.0f, 1.0f);

		// Second vector describes exterior bark color (if applicable)
		plantRGBACoeffs[1] = _mm_set_ps(0.86f, 0.6f, 0.69f, 1.0f);

		// Third vector describes exterior vascular coloration (relevant to vascular stems/branches, also leaves)
		plantRGBACoeffs[2] = _mm_set_ps(0.134f, 0.87f, 0.43f, 1.0f);

		// Fourth vector (temporarily) describes diffuse/specular weighting (x, y)
		plantRGBACoeffs[3] = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);

		// Remaining vectors will be PDE constants (more research needed)

		plants[i] = SceneFigure(_mm_set_ps(1, 0, 0, 0),
								_mm_set_ps(givenRadius, givenRadius, givenRadius, givenRadius), // Assume planets are spherical for now...
								_mm_set_ps(1, 0, 0, 0),
								_mm_set_ps(1, 0, 0, 0),
								1.0,
								(fourByteUnsigned)FIG_TYPES::PLANT,
								plantDistCoeffs,
								plantRGBACoeffs,
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
