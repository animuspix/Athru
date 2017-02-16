#include "Material.h"

Material::Material() : sonicData(Sound()), 
					   color{1, 1, 1, 1} {}

Material::Material(Sound sonicStuff,
				   float r, float g, float b, float a) :
				   sonicData(sonicStuff),
				   color{ r, g, b, a } 
{
	// Register appropriate shaders here...
}

Material::~Material()
{
}

float* Material::GetColorData()
{
	return color;
}