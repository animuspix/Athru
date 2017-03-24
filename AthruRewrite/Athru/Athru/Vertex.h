#pragma once

#include <directxmath.h>

struct Vertex
{
	Vertex() {};
	Vertex(DirectX::XMFLOAT4 pos,
		   DirectX::XMFLOAT4 baseColor,
		   DirectX::XMFLOAT4 norm,
		   DirectX::XMFLOAT2 UV) :
		   position{ pos.x, pos.y, pos.z, 1 },
		   color{ baseColor.x, baseColor.y, baseColor.z, baseColor.w },
		   normal{ norm.x, norm.y, norm.z, 1 },
		   texCoord{ UV.x, UV.y } {}
	
	DirectX::XMFLOAT4 position;
	DirectX::XMFLOAT4 color;
	DirectX::XMFLOAT4 normal;
	DirectX::XMFLOAT2 texCoord;

	void* operator new(size_t size);
	void operator delete(void* target);
};