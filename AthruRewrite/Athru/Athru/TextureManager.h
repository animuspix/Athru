#pragma once

#include "Typedefs.h"
#include "d3d11.h"
#include "AthruTexture2D.h"

#define MAX_TEXTURE_SIZE_2D (4096 * 4096)
#define MAX_TEXTURE_SIZE_3D (2048 * 2048 * 2048)

enum class AVAILABLE_EXTERNAL_TEXTURES
{
	BLANK_WHITE,
	IMPORTED_MESH_TEXTURE,
	NULL_TEXTURE,
};

enum class AVAILABLE_INTERNAL_TEXTURES
{
	SCREEN_TEXTURE,
	NULL_TEXTURE
};

class TextureManager
{
	public:
		TextureManager(ID3D11Device* d3dDevice);
		~TextureManager();

		AthruTexture2D& GetExternalTexture2D(AVAILABLE_EXTERNAL_TEXTURES textureID);
		AthruTexture2D& GetInternalTexture2D(AVAILABLE_INTERNAL_TEXTURES textureID);
		//AthruTexture3D& GetInternalTexture3D(AVAILABLE_EXTERNAL_TEXTURES textureID);

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// Array of available external textures (loaded from file)
		AthruTexture2D availableExternalTextures[(byteUnsigned)AVAILABLE_EXTERNAL_TEXTURES::NULL_TEXTURE];

		// Array of available internal textures (procedurally generated)
		AthruTexture2D availableInternalTextures[(byteUnsigned)AVAILABLE_INTERNAL_TEXTURES::NULL_TEXTURE];

		// Array of filepaths for available external textures, stored in the same order
		// as the elements within [availableExternalTextures]
		LPCWSTR textureLocations[(byteUnsigned)AVAILABLE_EXTERNAL_TEXTURES::NULL_TEXTURE];
};

