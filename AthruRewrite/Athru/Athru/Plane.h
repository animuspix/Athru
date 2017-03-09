#pragma once

#include <directxmath.h>

struct Plane
{
	DirectX::XMVECTOR cornerPos;
	DirectX::XMVECTOR normal;
	DirectX::XMVECTOR innerVector;
};

