#include "UtilityServiceCentre.h"
#include "Star.h"

Star::Star(float givenRadius,
		   DirectX::XMFLOAT3 position,
		   DirectX::XMVECTOR* distCoeffs) :
	  SceneFigure(position, _mm_set_ps(1, 0, 0, 0),
				  givenRadius, distCoeffs)
{}

Star::~Star()
{
}

// Push constructions for this class through Athru's custom allocator
void* Star::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<Star>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void Star::operator delete(void* target)
{
	return;
}