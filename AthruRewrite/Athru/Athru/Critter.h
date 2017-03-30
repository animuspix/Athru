#pragma once

#include "Material.h"
#include "SQT.h"

class Critter
{
	public:
		Critter();
		~Critter();

		SQT& GetTorsoTransformations();
		Material& GetCritterMaterial();

	private:
		// Just support snakes for now, we can broaden
		// support later
		// [torsoTransformations] must contain a position
		// matching a particular boxecule within the
		// scene
		SQT torsoTransformations;
		byteUnsigned limbCount;
		Material critterMaterialData;
};

