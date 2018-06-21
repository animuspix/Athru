#include "UtilityServiceCentre.h"
#include "Critter.h"
#include "Planet.h"

Planet::Planet(float givenScale,
			   DirectX::XMFLOAT3 position, DirectX::XMVECTOR qtnRotation,
			   DirectX::XMVECTOR* distCoeffs) :
		SceneFigure(position,
					_mm_set_ps(1, 0, 0, 0), givenScale,
					(fourByteUnsigned)FIG_TYPES::PLANET,
					distCoeffs)
{
	// Initialise plants
	plants = new SceneFigure[SceneStuff::MAX_NUM_SCENE_ORGANISMS];
	for (fourByteUnsigned i = 0; i < SceneStuff::MAX_NUM_SCENE_ORGANISMS; i += 1)
	{
		// Surface properties
		// (= branch placement/growth functions, PDE constants)
		DirectX::XMVECTOR plantDistCoeffs[3];

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
		plantDistCoeffs[1] = _mm_set_ps((float)(PlantStuff::MAX_BRANCH_COUNT_AT_FORK / ((rand() % PlantStuff::MAX_BRANCH_COUNT_AT_FORK) + 1)),
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

		plants[i] = SceneFigure(DirectX::XMFLOAT3(0, 0, 0),
								_mm_set_ps(1, 0, 0, 0),
								1.0,
								(fourByteUnsigned)FIG_TYPES::PLANT,
								plantDistCoeffs);
	}

	// Initialise critters
	// Seven days to create and validate procedural animals isn't really enough time...restrict them to spheres/cubes
	// for now
	critters = new SceneFigure[SceneStuff::MAX_NUM_SCENE_ORGANISMS];
	for (fourByteUnsigned i = 0; i < SceneStuff::MAX_NUM_SCENE_ORGANISMS; i += 1)
	{
		// Should edit this to carry cellular propagation modifiers in future...
		DirectX::XMVECTOR critterDistCoeffs[3] = { _mm_set_ps(0, 0, 0, 0),
												   _mm_set_ps(0, 0, 0, 0),
												   _mm_set_ps(0, 0, 0, 0) };

		critters[i] = SceneFigure(DirectX::XMFLOAT3(0, 0, 0),
								  _mm_set_ps(1, 0, 0, 0),
								  1.0,
								  (fourByteUnsigned)FIG_TYPES::CRITTER,
								  critterDistCoeffs);
	}
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
