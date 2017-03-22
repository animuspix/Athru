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
				 AVAILABLE_OBJECT_SHADERS shader0, AVAILABLE_OBJECT_SHADERS shader1,
				 AVAILABLE_OBJECT_SHADERS shader2, AVAILABLE_OBJECT_SHADERS shader3,
				 AVAILABLE_OBJECT_SHADERS shader4, LPCWSTR textureFilePath);
		~Material();

		// Retrieve the sound associated with [this]
		Sound& GetSonicData();

		// Retrieve the base color of [this]
		float* GetColorData();

		// Retrieve a shader-friendly interpretation
		// of the texture associated with [this]
		ID3D11ShaderResourceView* GetTextureAsShaderResource();

		// Retrieve the raw texture associated with [this]
		ID3D11Texture2D* GetTexture();

		// Retrieve the array of shaders associated with
		// [this]
		AVAILABLE_OBJECT_SHADERS* GetShaderSet();

	private:
		Sound sonicData;
		float color[4];

		ID3D11Texture2D* texture;
		ID3D11ShaderResourceView* shaderFriendlyTexture;

		AVAILABLE_OBJECT_SHADERS shaderSet[5];
};

