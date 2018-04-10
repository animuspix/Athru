#include "UtilityServiceCentre.h"
#include "System.h"

// System interactions are much more complicated than I thought; best to
// avoid actually simulating them for now and just stick to relatively
// simple static renders

System::System(DirectX::XMVECTOR sysPos)
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
	DirectX::XMVECTOR starRGBACoeffs[10] = { _mm_set_ps(1.0f,
														1.0f,
														1.0f,
														1.0f), // Surface color
											 _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f), // Diffuse/specular weighting (x, y)
											 _mm_set_ps(0.0f, 0.0f, 0.0f, /*(float)(rand()) + */6400000.0f), // Illuminant intensity stored in (x)
											 _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
											 _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
											 _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
											 _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f) };

	DirectX::XMVECTOR starPos = sysPos;
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
		float jParam = -0.6f;// + (((float)(rand() % 2) - 2.0f) / 10.0f); // Single parameter to keep shapes as clean and smooth as possible
		DirectX::XMVECTOR planetDistCoeffs[10] = { _mm_set_ps(0.0f, jParam, jParam, 0.0f), // First vector contains parameters (xyz) + w-slice (w) for the relevant quaternionic Julia fractal
												   _mm_set_ps(10.0f,
															  1.0f / (rand() % 10),
															  1.0f / (rand() % 10),
															  1.0f / (rand() % 10)), // Second vector contains local rotational axis (xyz), rotational speed (w)
												   _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f), // Third vector contains orbital speed in [x], yet-to-be-defined terrain constants in [yzw]
												   _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
												   _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
												   _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
												   _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f) };

		// Sticking with grayscale colors until I set up a scale-agnostic planetary color function
		float grayscaleRGB = 1.0f;//0.734f / ((float)(rand() % 10) + 1.0f);
		DirectX::XMVECTOR planetRGBACoeffs[10] = { _mm_set_ps(0.0f, 0.1f, 0.1f, 0.8f), // Diffuse/specular/reflective weighting (x, y, z),
												   _mm_set_ps(1.0f,
															  grayscaleRGB,
															  grayscaleRGB,
															  grayscaleRGB), // Surface color; should probably be procedural in future tests
												   _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
												   _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
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

	// Translate systems as appropriate
	position = sysPos;
}

System::~System()
{
}

void System::Update()
{}

DirectX::XMVECTOR System::GetPos()
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
