#include "UtilityServiceCentre.h"
#include "Galaxy.h"

Galaxy::Galaxy(AVAILABLE_GALACTIC_LAYOUTS galacticLayout)
{
	// Super, super unfinished; consider editing towards more
	// realistic stellar distributions in the future

	// If the chosen galactic layout is spherical, place stars
	// at random distances from the origin
	if (galacticLayout == AVAILABLE_GALACTIC_LAYOUTS::SPHERE)
	{
		// Guarantee that at least one system starts from the origin
		systems[0] = new System(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));

		// Generate remaining systems
		for (u4Byte i = 1; i < SceneStuff::SYSTEM_COUNT; i += 1)
		{
			systems[i] = new System(DirectX::XMFLOAT3((float)(rand()), (float)(rand()), (float)(rand())));
		}
	}

	// If galaxy is expected to have a spiral layout instead, place
	// stars within an approximate spiral curve (+ apply noise so that
	// the spiral arms are "fuzzy" rather than perfectly straight)
	else if (galacticLayout == AVAILABLE_GALACTIC_LAYOUTS::SPIRAL)
	{
		// Unit circle-involute (not /really/ a spiral, but close enough
		// for now): x = t * sin(t) + cos(t), y = sin(t) - cos(t)

		for (u4Byte i = 0; i < SceneStuff::SYSTEM_COUNT; i += 1)
		{
			systems[i] = new System(DirectX::XMFLOAT3((float)rand(), (float)(sin(i) - cos(i)), (float)((i * sin(i)) + cos(i))));
		}
	}
}

Galaxy::~Galaxy()
{

}

System** Galaxy::GetSystems()
{
	return systems;
}

System* Galaxy::GetCurrentSystem(DirectX::XMVECTOR& cameraPos)
{
	u4Byte systemIndex = 0;
	float lastDistToSystemCentre = FLT_MAX;
	for (u4Byte i = 0; i < SceneStuff::SYSTEM_COUNT; i += 1)
	{
		DirectX::XMFLOAT3 currSysPos = systems[systemIndex]->GetPos();
		DirectX::XMVECTOR cameraToSystemDiff = _mm_sub_ps(_mm_set_ps(0.0f,
																	 currSysPos.z,
																	 currSysPos.y,
																	 currSysPos.x),
														  cameraPos);
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
	StackAllocator* allocator = AthruCore::Utility::AccessMemory();
	return allocator->AlignedAlloc(size, (uByte)std::alignment_of<Galaxy>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void Galaxy::operator delete(void* target)
{
	return;
}
