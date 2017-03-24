#pragma once

#include "Mesh.h"

#define RECT_INDEX_COUNT 6

class AthruRect : public Mesh
{
	public:
		AthruRect() {}
		AthruRect(Material rectMaterial,
				  float baseWidth, float baseHeight);
		~AthruRect();
};

