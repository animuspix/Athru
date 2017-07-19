#include "UtilityServiceCentre.h"
#include "Star.h"

Star::Star(float givenRadius, DirectX::XMFLOAT4 givenColor) :
		   radius(givenRadius),
		   color{ givenColor.x, givenColor.y, givenColor.z, givenColor.w } {}

Star::~Star()
{
}

float Star::GetRadius()
{
	return radius;
}

DirectX::XMFLOAT4 Star::GetColor()
{
	return color;
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