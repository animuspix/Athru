#pragma once

#include "Mesh.h"

#define BOXECULE_INDEX_COUNT 36

class Boxecule : public Mesh
{
	public:
		Boxecule() {}
		Boxecule(Material boxeculeMaterial);
		~Boxecule();
};

