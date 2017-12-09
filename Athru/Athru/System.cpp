#include "UtilityServiceCentre.h"
#include "System.h"

// System interactions are much more complicated than I thought; best to
// avoid actually simulating them for now and just stick to relatively
// simple static renders

System::System()
{
	// Temp distance coefficients (star)
	DirectX::XMVECTOR starDistCoeffs[10] = { _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
											 _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
											 _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
											 _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
											 _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f) };

	// Create local star
	// Some sort of procedural star generator would probably be nice here;
	// it'd be easier to expose to a galaxy/system editor and could produce
	// much more interesting results than a fixed star size with random
	// color components
	DirectX::XMVECTOR starRGBACoeffs[10] = { _mm_set_ps(1.0f / (rand() % 255), 1.0f / (rand() % 255), 1.0f / (rand() % 255), 1.0f), // Surface color
											 _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f), // Diffuse/specular weighting (x, y)
											 _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
											 _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
											 _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
											 _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f) };

	DirectX::XMVECTOR starPos = _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f);
	float starRadius = 400.0f;
	star = new Star(starRadius,
					starPos,
					starDistCoeffs,
					starRGBACoeffs);

	// Create local planets
	for (int i = 0; i < (SceneStuff::MAX_NUM_SCENE_FIGURES - 1); i += 1)
	{
		// Temp distance coefficients (planets) + color properties
		// Most interesting Julias seem to have negative [xyz] parameters, so stick to those here
		float jParam = -0.5f + (((float)(rand() % 2) - 2.0f) / 10.0f); // Single parameter to keep shapes as clean and smooth as possible
		DirectX::XMVECTOR planetDistCoeffs[10] = { _mm_set_ps(jParam, jParam, jParam, 0.0f), // First vector contains parameters (xyz) + w-slice (w) for the relevant quaternionic Julia fractal
												   _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
												   _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
												   _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
												   _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
												   _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f) };

		DirectX::XMVECTOR planetRGBACoeffs[10] = { _mm_set_ps(0.8f, 0.1f, 0.1f, 0.0f), // Diffuse/specular/reflective weighting (x, y),
												   _mm_set_ps(0.234f * ((float)(rand() % 4) + 1.0f),
															  0.507f * ((float)(rand() % 2) + 1.0f),
															  0.734f / ((float)(rand() % 10) + 1.0f), 1.0f), // Surface color; should probably be procedural in future tests
												   _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
												   _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
												   _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
												   _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f) };

		float radius = 100.0f;//(float)((rand() % 100) + 50); // Should introduce more accurate variance here
		float offset = 1.5f;
		planets[i] = new Planet(radius,
								_mm_add_ps(_mm_set_ps(0,
													  /*(radius * offset) + starRadius*/0, // Z-offsets are good, but hard to view with 0.1FPS performance :P
													  0,
													  (starRadius * float(i + 2))),
													  starPos),
								_mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f),
								planetDistCoeffs, planetRGBACoeffs);
	}

	// Place systems at the world-space origin by default
	position = _mm_set_ps(1, 0, 0, 0);
}

System::~System()
{
}

void System::Update()
{}

DirectX::XMVECTOR& System::FetchPos()
{
	return position;
}

Star* System::GetStar()
{
	return star;
}

Planet** System::GetPlanets()
{
	return planets;
}

// Push constructions for this class through Athru's custom allocator
void* System::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<System>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void System::operator delete(void* target)
{
	return;
}

// Push constructions for this class through Athru's custom allocator
void* System::operator new[](size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<System>(), false);
}

// We aren't expecting to use [delete[]], so overload it to do nothing
void System::operator delete[](void* target)
{
	return;
}
