#pragma once

#include <directxmath.h>

struct SceneVertex
{
	SceneVertex() {};
	SceneVertex(DirectX::XMFLOAT4 pos,
				DirectX::XMFLOAT3 UV) :
				position{ pos.x, pos.y, pos.z, 1 },
				texCoord{ UV.x, UV.y, UV.z } {}

	DirectX::XMFLOAT4 position;
	DirectX::XMFLOAT3 texCoord;

	void* operator new(size_t size);
	void operator delete(void* target);
};