#include "UtilityServiceCentre.h"
#include "Galaxy.h"

Galaxy::Galaxy(AVAILABLE_GALACTIC_LAYOUTS galacticLayout)
{
	systems = new System[SceneStuff::SYSTEM_COUNT];

	// Super, super unfinished; consider editing towards more
	// realistic stellar distributions in the future

	// If the chosen galactic layout is spherical, place stars
	// at random distances from the origin
	fourByteUnsigned tempThresh = 200;
	if (galacticLayout == AVAILABLE_GALACTIC_LAYOUTS::SPHERE)
	{
		for (fourByteUnsigned i = 0; i < SceneStuff::SYSTEM_COUNT; i += 1)
		{
			systems[i].FetchPos() = _mm_set_ps((float)(rand() % tempThresh), (float)(rand() % tempThresh), (float)(rand() % tempThresh), 1.0f);
		}
	}

	// If galaxy is expected to have a spiral layout instead, place
	// stars within an approximate spiral curve (+ apply noise so that
	// the spiral arms are "fuzzy" rather than perfectly straight)
	else if (galacticLayout == AVAILABLE_GALACTIC_LAYOUTS::SPIRAL)
	{
		// Unit circle-involute (not /really/ a spiral, but close enough
		// for now): x = t * sin(t) + cos(t), y = sin(t) - cos(t)

		for (fourByteUnsigned i = 0; i < SceneStuff::SYSTEM_COUNT; i += 1)
		{
			systems[i].FetchPos() = _mm_set_ps(1.0f, (float)rand(), (float)(sin(i) - cos(i)), (float)((i * sin(i)) + cos(i)));
		}
	}
}

Galaxy::~Galaxy()
{

}

void Galaxy::Update()
{
	for (fourByteUnsigned i = 0; i < SceneStuff::SYSTEM_COUNT; i += 1)
	{
		systems[i].Update();
	}
}

System& Galaxy::GetCurrentSystem(DirectX::XMVECTOR& cameraPos)
{
	fourByteUnsigned systemIndex = 0;
	float lastDistToSystemCentre = FLT_MAX;
	for (fourByteUnsigned i = 0; i < SceneStuff::SYSTEM_COUNT; i += 1)
	{
		DirectX::XMVECTOR cameraToSystemDiff = _mm_sub_ps(systems[systemIndex].FetchPos(), cameraPos);
		float distToSystemCentre = _mm_cvtss_f32(DirectX::XMVector3Length(cameraToSystemDiff));
		if (distToSystemCentre < lastDistToSystemCentre)
		{
			lastDistToSystemCentre = distToSystemCentre;
			systemIndex = i;
		}
	}

	return systems[systemIndex];
}

// Push constructions for this class through Athru's custom allocator
void* Galaxy::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<Galaxy>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void Galaxy::operator delete(void* target)
{
	return;
}
