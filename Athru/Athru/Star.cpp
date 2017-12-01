#include "UtilityServiceCentre.h"
#include "Star.h"

Star::Star(float givenRadius,
		   DirectX::XMVECTOR* distCoeffs, DirectX::XMVECTOR* rgbaCoeffs) :
	  SceneFigure(_mm_set_ps(0, 0, 0, 0), _mm_set_ps(0, 1, 1, 1), _mm_set_ps(1, 0, 0, 0),
				  _mm_set_ps(1, 0, 0, 0), 1, (fourByteUnsigned)FIG_TYPES::STAR, distCoeffs, rgbaCoeffs, 1)
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