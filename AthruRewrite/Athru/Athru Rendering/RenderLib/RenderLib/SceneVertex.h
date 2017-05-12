#pragma once

#include <directxmath.h>

struct SceneVertex
{
	SceneVertex() {};
	SceneVertex(DirectX::XMFLOAT4 pos) :
				position{ pos.x, pos.y, pos.z, 1 } {}

	// EVERYTHING is stored in 3D textures, so no need for
	// anything other than position in each vertex
	DirectX::XMFLOAT4 position;

	void* operator new(size_t size);
	void operator delete(void* target);
};