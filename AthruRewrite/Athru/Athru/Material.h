#pragma once

#include "Typedefs.h"
#include "RenderManager.h"
#include "AthruTexture2D.h"
#include "Luminance.h"
#include "Sound.h"

class Material
{
	public:
		Material();
		Material(Sound sonicStuff,
				 Luminance lightStuff,
				 float r, float g, float b, float a,
				 float baseRoughness,
				 float baseReflectiveness,
				 AVAILABLE_OBJECT_SHADERS objectShader,
				 AthruTexture2D baseTexture);
		~Material();

		// Retrieve the sound associated with [this]
		Sound& GetSonicData();

		// Retrieve the base color of [this]
		DirectX::XMFLOAT4& GetColorData();

		// Retrieve the roughness of [this]
		float& GetRoughness();

		// Retrieve the reflectiveness of [this]
		float& GetReflectiveness();

		// Retrieve the illumination data associated with [this]
		Luminance& GetLightData();

		// Retrieve the raw texture associated with [this]
		AthruTexture2D& GetTexture();

		// Retrieve the raw texture associated with [this]
		void SetTexture(AthruTexture2D suppliedTexture);

		// Retrieve the shader associated with [this]
		AVAILABLE_OBJECT_SHADERS GetShader();

	private:
		// Reflectiveness should really expand into its own struct
		// with properties for reflection and refraction, but I don't
		// have the time to do that and I'd prefer to write a working
		// light system that only works with opaque materials than an
		// unfinished lighting system that works with nothing :P
		// That said, I'll look into adding it after submission
		// (since this is doubling as an assignment project)

		Sound sonicData;
		Luminance lightData;
		DirectX::XMFLOAT4 color;
		float roughness;
		float reflectiveness;
		AthruTexture2D texture;
		AVAILABLE_OBJECT_SHADERS shader;
};

