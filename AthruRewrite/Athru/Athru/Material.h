#pragma once

#include "Typedefs.h"
#include "RenderManager.h"
#include "Sound.h"

class Material
{
	public:
		Material();
		Material(Sound sonicStuff,
				 float r, float g, float b, float a,
				 AVAILABLE_SHADERS shader0, AVAILABLE_SHADERS shader1,
				 AVAILABLE_SHADERS shader2, AVAILABLE_SHADERS shader3,
				 AVAILABLE_SHADERS shader4);
		~Material();

		// Retrieve the sound associated with [this]
		Sound& GetSonicData();

		// Retrieve the base color of [this]
		float* GetColorData();

		// Retrieve the array of shaders associated with
		// [this]
		AVAILABLE_SHADERS* GetShaderSet();

	private:
		Sound sonicData;
		AVAILABLE_SHADERS shaderSet[5];
		float color[4];
};

