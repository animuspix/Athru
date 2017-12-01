#include "UtilityServiceCentre.h"
#include "System.h"

// System interactions are much more complicated than I thought; best to
// avoid actually simulating them for now and just stick to relatively
// simple static renders

System::System()
{
	// No mineral/chemistry library, so just assume
	// planets are made of water :P

	// Temp distance coefficients + color properties
	DirectX::XMVECTOR distCoeffs[10] = { _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
										 _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
										 _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
										 _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
										 _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f) };

	DirectX::XMVECTOR planetRGBACoeffs[10] = { _mm_set_ps(0.234f, 0.507f, 0.734f, 1.0f), // Surface color
											   _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f), // Diffuse/specular weighting (x, y)
											   _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
											   _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
											   _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f),
											   _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f), _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f) };

	// Create local planets
	float radiusZero = 1.0f;
	float radiusZeroCube = radiusZero * radiusZero * 50000.0f;
	float fourThirds = 4.0f / 3.0f;
	float massZero = fourThirds * (MathsStuff::PI * radiusZeroCube);
	float orbitalOffsetZero = radiusZero * 1.5f;
	planets[0] = new Planet(massZero, radiusZero,
							_mm_set_ps(0, 0, radiusZero * 1.5f, radiusZero * 1.5f),
							_mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f), distCoeffs, planetRGBACoeffs);

	float radiusOne = 100;
	float radiusOneCube = radiusOne * radiusOne * radiusOne;
	float massOne = fourThirds * (MathsStuff::PI * radiusOneCube);
	float orbitalOffsetOne = orbitalOffsetZero + radiusOne * 1.5f;
	planets[1] = new Planet(massOne, radiusOne,
							_mm_set_ps(0, 0, orbitalOffsetOne, orbitalOffsetOne),
							_mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f), distCoeffs, planetRGBACoeffs);

	float radiusTwo = 120;
	float radiusTwoCube = radiusTwo * radiusTwo * radiusTwo;
	float massTwo = fourThirds * (MathsStuff::PI * radiusTwoCube);
	float orbitalOffsetTwo = orbitalOffsetOne + radiusTwo * 1.5f;
	planets[2] = new Planet(massOne, radiusOne,
							_mm_set_ps(0, 0, orbitalOffsetTwo, orbitalOffsetTwo),
							_mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f), distCoeffs, planetRGBACoeffs);

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
	star = new Star(2000000.0f, distCoeffs, starRGBACoeffs);

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
