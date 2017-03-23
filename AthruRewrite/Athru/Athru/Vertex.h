#pragma once

#include <directxmath.h>

struct Vertex
{
	Vertex() {};
	Vertex(float x, float y, float z,
		   float* baseColor,
		   float normX, float normY, float normZ,
		   float texCoordU, float texCoordV) :
		   position{ x, y, z, 1 },
		   color{ baseColor[0], baseColor[1], baseColor[2], baseColor[3] },
		   normal{ normX, normY, normZ, 1 },
		   texCoord(texCoordU, texCoordV) {}

	DirectX::XMFLOAT4 position;
	DirectX::XMFLOAT4 color;
	DirectX::XMFLOAT4 normal;
	DirectX::XMFLOAT2 texCoord;
};