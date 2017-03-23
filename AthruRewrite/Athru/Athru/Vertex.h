#pragma once

#include <directxmath.h>

struct Vertex
{
	Vertex() {};
	Vertex(float x, float y, float z,
		   float* baseColor) :
		   position{ x, y, z, 1 },
		   color{ baseColor[0], baseColor[1], baseColor[2], baseColor[3] } {}

	DirectX::XMFLOAT4 position;
	DirectX::XMFLOAT4 color;
};