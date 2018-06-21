#include "SceneFigure.h"

SceneFigure::SceneFigure()
{
	DirectX::XMVECTOR baseDistCoeffs[3] = { _mm_set_ps(0, 0, 0, 0),
											_mm_set_ps(0, 0, 0, 0),
											_mm_set_ps(0, 0, 0, 0) };

	coreFigure = Figure(DirectX::XMFLOAT4(0, 0, 0, 1.0f),
						_mm_set_ps(1, 0, 0, 0),
						(fourByteUnsigned)FIG_TYPES::CRITTER,
						baseDistCoeffs,
						this);
}

SceneFigure::SceneFigure(DirectX::XMFLOAT3 position,
						 DirectX::XMVECTOR qtnRotation, float scale,
						 fourByteUnsigned figType, DirectX::XMVECTOR* distCoeffs)
{
	coreFigure = Figure(DirectX::XMFLOAT4(position.x, position.y, position.z, scale),
						DirectX::XMQuaternionInverse(qtnRotation),
						(fourByteUnsigned)figType, distCoeffs,
						this);
}

SceneFigure::~SceneFigure()
{
}

FIG_TYPES SceneFigure::GetDistFuncType()
{
	return (FIG_TYPES)coreFigure.self.x;
}

void SceneFigure::SetRotation(DirectX::XMVECTOR axis,
							  float angle)
{
	// Convert the given rotation into the internal [Figure]'s
	// quaternion representation; also invert it beforehand since
	// rotating rays is much easier than rotating abstract
	// distance functions
	DirectX::XMVECTOR invAxisAngle = DirectX::XMQuaternionRotationAxis(axis,
																	   angle);

	coreFigure.rotationQtn = DirectX::XMQuaternionInverse(invAxisAngle);
}

DirectX::XMVECTOR SceneFigure::GetQtnRotation()
{
	return coreFigure.rotationQtn;
}

SceneFigure::Figure SceneFigure::GetCoreFigure()
{
	return coreFigure;
}

void SceneFigure::SetCoreFigure(Figure& fig)
{
	// Store the given core figure
	coreFigure = fig;
}

// Push constructions for this class through Athru's custom allocator
void* SceneFigure::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<SceneFigure>(), false);
}

void* SceneFigure::operator new[](size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<SceneFigure>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing;
void SceneFigure::operator delete(void* target)
{
	return;
}

// We aren't expecting to use [delete[]], so overload it to do nothing;
void SceneFigure::operator delete[](void* target)
{
	return;
}
