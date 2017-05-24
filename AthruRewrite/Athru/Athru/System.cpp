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
	float radiusZero = 50.0f;
	float radiusZeroCube = radiusZero * radiusZero * 50000.0f;
	float fourThirds = 4.0f / 3.0f;
	float massZero = fourThirds * (MathsStuff::PI * radiusZeroCube);
	float orbitalOffsetZero = radiusZero * 1.5f;
	float archetypeWeights[(byteUnsigned)AVAILABLE_PLANET_ARCHETYPES::NULL_ARCHETYPE] = { 1.0f };
	planets[0] = new Planet(massZero, radiusZero,
							DirectX::XMFLOAT4(1, 1, 1, 1), _mm_set_ps(0, 0, radiusZero * 1.5f, radiusZero * 1.5f),
							DirectX::XMFLOAT3(0, 0, 0),
						    archetypeWeights);

	float radiusOne = 100;
	float radiusOneCube = radiusOne * radiusOne * radiusOne;
	float massOne = fourThirds * (MathsStuff::PI * radiusOneCube);
	float orbitalOffsetOne = orbitalOffsetZero + radiusOne * 1.5f;
	planets[1] = new Planet(massOne, radiusOne,
							DirectX::XMFLOAT4(1, 0.5f, 1, 1), _mm_set_ps(0, 0, orbitalOffsetOne, orbitalOffsetOne),
							DirectX::XMFLOAT3(0, 0, 0),
						    archetypeWeights);

	float radiusTwo = 120;
	float radiusTwoCube = radiusTwo * radiusTwo * radiusTwo;
	float massTwo = fourThirds * (MathsStuff::PI * radiusTwoCube);
	float orbitalOffsetTwo = orbitalOffsetOne + radiusTwo * 1.5f;
	planets[2] = new Planet(massOne, radiusOne,
							DirectX::XMFLOAT4(1, 1, 0.5f, 1), _mm_set_ps(0, 0, orbitalOffsetTwo, orbitalOffsetTwo),
							DirectX::XMFLOAT3(0, 0, 0),
						    archetypeWeights);

	// Create local star
	// Some sort of procedural star generator would probably be nice here;
	// it'd be easier to expose to a galaxy/system editor and could produce
	// much more interesting results than a fixed star size with random
	// color components
	star = new Star(2000000.0f, DirectX::XMFLOAT4(1.0f / (rand() % 255), 1.0f / (rand() % 255), 1.0f / (rand() % 255), 1));

	// Place systems at the world-space origin by default
	position = _mm_set_ps(1, 0, 0, 0);

}

System::~System()
{
}

void System::Update()
{
	// Not really any time to think up a
	// procedural orbital physics simulator,
	// so just spin each planet in place instead
	float speed = 0.01f;
	planets[0]->Rotate(DirectX::XMFLOAT3(0.0f, 0.0f, speed * TimeStuff::deltaTime()));
	planets[1]->Rotate(DirectX::XMFLOAT3(0.0f, speed * TimeStuff::deltaTime(), 0.0f));
	planets[2]->Rotate(DirectX::XMFLOAT3(speed * TimeStuff::deltaTime(), 0.0f, 0.0f));
}

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
