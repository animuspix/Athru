#include "UtilityServiceCentre.h"
#include "System.h"

// System interactions are much more complicated than I thought; best to
// avoid actually simulating them for now and just stick to relatively
// simple static renders

System::System()
{
	// No mineral/chemistry library, so just assume
	// planets are made of water :P

	// Create local planets
	float radiusZero = 1.0f;
	float radiusZeroCube = radiusZero * radiusZero * 50000.0f;
	float fourThirds = 4.0f / 3.0f;
	float massZero = fourThirds * (MathsStuff::PI * radiusZeroCube);
	float orbitalOffsetZero = radiusZero * 1.5f;
	planets[0] = new Planet(massZero, radiusZero,
							_mm_set_ps(1, 1, 1, 1), _mm_set_ps(0, 0, radiusZero * 1.5f, radiusZero * 1.5f),
							DirectX::XMFLOAT3(0, 0, 0));

	float radiusOne = 100;
	float radiusOneCube = radiusOne * radiusOne * radiusOne;
	float massOne = fourThirds * (MathsStuff::PI * radiusOneCube);
	float orbitalOffsetOne = orbitalOffsetZero + radiusOne * 1.5f;
	planets[1] = new Planet(massOne, radiusOne,
							_mm_set_ps(1, 0.5f, 1, 1), _mm_set_ps(0, 0, orbitalOffsetOne, orbitalOffsetOne),
							DirectX::XMFLOAT3(0, 0, 0));

	float radiusTwo = 120;
	float radiusTwoCube = radiusTwo * radiusTwo * radiusTwo;
	float massTwo = fourThirds * (MathsStuff::PI * radiusTwoCube);
	float orbitalOffsetTwo = orbitalOffsetOne + radiusTwo * 1.5f;
	planets[2] = new Planet(massOne, radiusOne,
							_mm_set_ps(1, 1, 0.5f, 1), _mm_set_ps(0, 0, orbitalOffsetTwo, orbitalOffsetTwo),
							DirectX::XMFLOAT3(0, 0, 0));

	// Create local star
	// Some sort of procedural star generator would probably be nice here;
	// it'd be easier to expose to a galaxy/system editor and could produce
	// much more interesting results than a fixed star size with random
	// color components
	star = new Star(2000000.0f, DirectX::XMFLOAT4(1.0f / (rand() % 255), 1.0f / (rand() % 255), 1.0f / (rand() % 255), 1));

	// Place systems at the world-space origin by default
	position = _mm_set_ps(1, 0, 0, 0);

	// Apply initial axial spins to each planet in the system
	float speed = 0.01f;
	planets[0]->BoostAngularVelo(_mm_set_ps(0.0f, 0.0f, speed * TimeStuff::deltaTime(), 0.0f));
	planets[1]->BoostAngularVelo(_mm_set_ps(0.0f, 0.0f, speed * TimeStuff::deltaTime(), 0.0f));
	planets[2]->BoostAngularVelo(_mm_set_ps(0.0f, 0.0f, speed * TimeStuff::deltaTime(), 0.0f));
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
