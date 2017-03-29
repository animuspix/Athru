#include "ServiceCentre.h"
#include "Material.h"

Material::Material() : sonicData(Sound(0.5f, 0.5f)),
					   lightData(Luminance(0, AVAILABLE_LIGHTING_SHADERS::POINT_LIGHT)),
					   color{ 1.0f, 1.0f, 1.0f, 0.6f },
					   roughness(0),
					   reflectiveness(0),
					   shader(AVAILABLE_OBJECT_SHADERS::TEXTURED_RASTERIZER)
{
	texture = ServiceCentre::AccessTextureManager()->GetTexture(AVAILABLE_TEXTURES::BLANK_WHITE);
}

Material::Material(Sound sonicStuff,
				   Luminance lightStuff,
				   float r, float g, float b, float a,
				   float baseRoughness,
				   float baseReflectiveness,
				   AVAILABLE_OBJECT_SHADERS objectShader,
				   AthruTexture baseTexture) :
				   sonicData(sonicStuff),
				   lightData(lightStuff),
				   color{ r, g, b, a },
				   roughness(baseRoughness),
				   reflectiveness(baseReflectiveness),
				   shader(objectShader),
				   texture(baseTexture) {}

Material::~Material()
{
}

Sound& Material::GetSonicData()
{
	return sonicData;
}

DirectX::XMFLOAT4& Material::GetColorData()
{
	return color;
}

float& Material::GetRoughness()
{
	return roughness;
}

float& Material::GetReflectiveness()
{
	return reflectiveness;
}

Luminance& Material::GetLightData()
{
	return lightData;
}

AthruTexture& Material::GetTexture()
{
	return texture;
}

void Material::SetTexture(AthruTexture suppliedTexture)
{
	texture = suppliedTexture;
}

AVAILABLE_OBJECT_SHADERS Material::GetShader()
{
	return shader;
}
