#include "Material.h"

Material::Material() : sonicData(Sound()),
					   deferredShaderSet{ DEFERRED::AVAILABLE_OBJECT_SHADERS::RASTERIZER, DEFERRED::AVAILABLE_OBJECT_SHADERS::NULL_SHADER,
										  DEFERRED::AVAILABLE_OBJECT_SHADERS::NULL_SHADER, DEFERRED::AVAILABLE_OBJECT_SHADERS::NULL_SHADER,
										  DEFERRED::AVAILABLE_OBJECT_SHADERS::NULL_SHADER },
					   forwardShaderSet{ FORWARD::AVAILABLE_OBJECT_SHADERS::NULL_SHADER, FORWARD::AVAILABLE_OBJECT_SHADERS::NULL_SHADER,
										 FORWARD::AVAILABLE_OBJECT_SHADERS::NULL_SHADER, FORWARD::AVAILABLE_OBJECT_SHADERS::NULL_SHADER,
										 FORWARD::AVAILABLE_OBJECT_SHADERS::NULL_SHADER },
					   color{ 1.0f, 1.0f, 1.0f, 0.6f }
{
	texture = AthruTexture();
}

Material::Material(Sound sonicStuff,
				   float r, float g, float b, float a,
				   DEFERRED::AVAILABLE_OBJECT_SHADERS deferredShader0, DEFERRED::AVAILABLE_OBJECT_SHADERS deferredShader1,
				   DEFERRED::AVAILABLE_OBJECT_SHADERS deferredShader2, DEFERRED::AVAILABLE_OBJECT_SHADERS deferredShader3,
				   DEFERRED::AVAILABLE_OBJECT_SHADERS deferredShader4, FORWARD::AVAILABLE_OBJECT_SHADERS forwardShader0,
				   FORWARD::AVAILABLE_OBJECT_SHADERS forwardShader1, FORWARD::AVAILABLE_OBJECT_SHADERS forwardShader2,
				   FORWARD::AVAILABLE_OBJECT_SHADERS forwardShader3, FORWARD::AVAILABLE_OBJECT_SHADERS forwardShader4,
				   AthruTexture baseTexture) :
				   sonicData(sonicStuff),
				   color{ r, g, b, a },
				   deferredShaderSet{ deferredShader0, deferredShader1, deferredShader2, deferredShader3, deferredShader4 },
				   forwardShaderSet{ forwardShader0, forwardShader1, forwardShader2, forwardShader3, forwardShader4 } {}

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

AthruTexture& Material::GetTexture()
{
	return texture;
}

void Material::SetTexture(AthruTexture suppliedTexture)
{
	texture = suppliedTexture;
}

DEFERRED::AVAILABLE_OBJECT_SHADERS* Material::GetDeferredShaderSet()
{
	return deferredShaderSet;
}

FORWARD::AVAILABLE_OBJECT_SHADERS* Material::GetForwardShaderSet()
{
	return forwardShaderSet;
}
