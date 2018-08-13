#pragma once

#include "directxmath.h"

struct PostVertex
{
	PostVertex() {};
	PostVertex(DirectX::XMFLOAT4 pos,
				DirectX::XMFLOAT2 UV) :
				position{ pos.x, pos.y, pos.z, 1 },
				texCoord{ UV.x, UV.y } {}

	DirectX::XMFLOAT4 position;
	DirectX::XMFLOAT2 texCoord;

	void* operator new(size_t size);
	void operator delete(void* target);
};

