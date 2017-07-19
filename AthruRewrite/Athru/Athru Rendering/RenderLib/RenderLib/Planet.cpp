#include "UtilityServiceCentre.h"
#include "Planet.h"

Planet::Planet(float givenMass, float givenRadius, DirectX::XMFLOAT4 givenAvgColor,
			   DirectX::XMVECTOR offsetFromStar, DirectX::XMFLOAT3 rotation) :
			   mass(givenMass),
			   radius(givenRadius),
			   avgColor{ givenAvgColor.x, givenAvgColor.y, givenAvgColor.z, givenAvgColor.w },
			   stellarOffset(offsetFromStar),
			   eulerRotation(rotation)
{
	quaternionRotation = DirectX::XMQuaternionRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
}

Planet::~Planet()
{
}

float Planet::GetMass()
{
	return mass;
}

float Planet::GetRadius()
{
	return radius;
}

DirectX::XMFLOAT4 Planet::GetAvgColor()
{
	return avgColor;
}

DirectX::XMVECTOR& Planet::FetchStellarOffset()
{
	return stellarOffset;
}

DirectX::XMVECTOR Planet::GetRotationQuaternion()
{
	return quaternionRotation;
}

void Planet::Rotate(DirectX::XMFLOAT3 rotateBy)
{
	eulerRotation.x += rotateBy.x;
	eulerRotation.y += rotateBy.y;
	eulerRotation.z += rotateBy.z;
	quaternionRotation = DirectX::XMQuaternionRotationRollPitchYaw(eulerRotation.x, eulerRotation.y, eulerRotation.z);
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
