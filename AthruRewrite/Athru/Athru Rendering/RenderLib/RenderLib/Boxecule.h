#pragma once

#define BOXECULE_INDEX_COUNT 36

#include "Mesh.h"

class Boxecule : public Mesh
{
	public:
		Boxecule(ID3D11Device* device);
		~Boxecule();
};

