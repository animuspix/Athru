#include "Material.h"

Material::Material() : sonicData(Sound()),
					   shaderSet{ AVAILABLE_SHADERS::RASTERIZER, AVAILABLE_SHADERS::NULL_SHADER,
								  AVAILABLE_SHADERS::NULL_SHADER, AVAILABLE_SHADERS::NULL_SHADER,
								  AVAILABLE_SHADERS::NULL_SHADER },
					   color{ 1.0f, 1.0f, 1.0f, 0.6f } {}

Material::Material(Sound sonicStuff,
				   float r, float g, float b, float a,
				   AVAILABLE_SHADERS shader0, AVAILABLE_SHADERS shader1,
				   AVAILABLE_SHADERS shader2, AVAILABLE_SHADERS shader3,
				   AVAILABLE_SHADERS shader4) :
				   sonicData(sonicStuff),
				   color{ r, g, b, a },
				   shaderSet{ shader0, shader1, shader2, shader3, shader4 } {}

Material::~Material()
{
}

Sound& Material::GetSonicData()
{
	return sonicData;
}

float* Material::GetColorData()
{
	return color;
}

AVAILABLE_SHADERS* Material::GetShaderSet()
{
	return shaderSet;
}
