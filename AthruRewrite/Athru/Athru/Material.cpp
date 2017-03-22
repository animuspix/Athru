#include "Material.h"

Material::Material() : sonicData(Sound()),
					   shaderSet{ AVAILABLE_OBJECT_SHADERS::RASTERIZER, AVAILABLE_OBJECT_SHADERS::NULL_SHADER,
								  AVAILABLE_OBJECT_SHADERS::NULL_SHADER, AVAILABLE_OBJECT_SHADERS::NULL_SHADER,
								  AVAILABLE_OBJECT_SHADERS::NULL_SHADER },
					   color{ 1.0f, 1.0f, 1.0f, 0.6f } {}

Material::Material(Sound sonicStuff,
				   float r, float g, float b, float a,
				   AVAILABLE_OBJECT_SHADERS shader0, AVAILABLE_OBJECT_SHADERS shader1,
				   AVAILABLE_OBJECT_SHADERS shader2, AVAILABLE_OBJECT_SHADERS shader3,
				   AVAILABLE_OBJECT_SHADERS shader4,
				   LPCWSTR textureFilePath) :
				   sonicData(sonicStuff),
				   color{ r, g, b, a },
				   shaderSet{ shader0, shader1, shader2, shader3, shader4 }
{
	// Construct texture here...
	texture = nullptr;
	shaderFriendlyTexture = nullptr;
}

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

ID3D11ShaderResourceView* Material::GetTextureAsShaderResource()
{
	return shaderFriendlyTexture;
}

ID3D11Texture2D* Material::GetTexture()
{
	return texture;
}

AVAILABLE_OBJECT_SHADERS* Material::GetShaderSet()
{
	return shaderSet;
}
