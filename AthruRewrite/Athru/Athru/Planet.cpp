#include "UtilityServiceCentre.h"
#include "Planet.h"

Planet::Planet(float givenMass, float givenRadius, DirectX::XMFLOAT4 givenAvgColor,
			   DirectX::XMVECTOR offsetFromStar, DirectX::XMFLOAT3 rotation) :
		SceneFigure(_mm_set_ps(0, 0, 0, 0), _mm_set_ps(0, 0, 0, 0), _mm_set_ps(1, 0, 0, 0),
					_mm_set_ps(1, 0, 0, 0), givenRadius, _mm_set_ps(1, 1, 0, 1), FIG_TYPES::PLANET)
{}

Planet::~Planet()
{
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
