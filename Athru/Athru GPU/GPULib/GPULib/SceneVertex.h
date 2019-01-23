#pragma once

#include <directxmath.h>

struct SceneVertex
{
	SceneVertex() : position(DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f)) {};
	SceneVertex(DirectX::XMFLOAT4 pos) :
				position{ pos.x, pos.y, pos.z, 1 } {}

	// EVERYTHING is stored in 3D textures, so no need for
	// anything other than position in each scene vertex
	DirectX::XMFLOAT4 position;

	void* operator new(size_t size);
	void operator delete(void* target);
};