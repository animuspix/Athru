#pragma once

#include "Typedefs.h"
#include "ShaderCentre.h"
#include "Sound.h"

// Read the relevant tutorial, _then_ figure out how this plugs into the shader centre :)

class Material
{
	public:
		Material();
		Material(Sound sonicStuff,
				 float r, float g, float b, float a);
		~Material();
		
		// Retrieve the base color of [this]
		float* GetColorData();

	private:
		Sound sonicData;
		float color[4];
};

