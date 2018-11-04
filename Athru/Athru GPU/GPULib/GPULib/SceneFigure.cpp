#include "SceneFigure.h"

SceneFigure::SceneFigure()
{
	DirectX::XMVECTOR baseDistCoeffs[3] = { _mm_set_ps(0, 0, 0, 0),
											_mm_set_ps(0, 0, 0, 0),
											_mm_set_ps(0, 0, 0, 0) };

	coreFigure = Figure(DirectX::XMFLOAT4(0, 0, 0, 1.0f),
						baseDistCoeffs);
}

SceneFigure::SceneFigure(DirectX::XMFLOAT3 position, float scale,
						 DirectX::XMVECTOR* distCoeffs)
{
	coreFigure = Figure(DirectX::XMFLOAT4(position.x, position.y, position.z, scale),
						distCoeffs);
}

SceneFigure::~SceneFigure() {}

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
