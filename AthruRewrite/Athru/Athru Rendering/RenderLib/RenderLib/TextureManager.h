#pragma once

#include "Typedefs.h"
#include "d3d11.h"
#include "AthruTexture2D.h"
#include "AthruTexture3D.h"

#define MAX_TEXTURE_SIZE_2D (4096 * 4096)
#define MAX_TEXTURE_SIZE_3D (2048 * 2048 * 2048)

enum class AVAILABLE_DISPLAY_TEXTURES
{
	SCREEN_TEXTURE,
	BLUR_MASK,
	BRIGHTEN_MASK,
	DRUGS_MASK,
	NULL_TEXTURE
};

enum class AVAILABLE_VOLUME_TEXTURES
{
	SCENE_COLOR_TEXTURE,
	SCENE_NORMAL_TEXTURE,
	SCENE_PBR_TEXTURE,
	SCENE_EMISSIVITY_TEXTURE,
	NULL_TEXTURE
};

class TextureManager
{
	public:
		TextureManager(ID3D11Device* d3dDevice);
		~TextureManager();

		AthruTexture2D& GetDisplayTexture(AVAILABLE_DISPLAY_TEXTURES textureID);
		AthruTexture3D& GetVolumeTexture(AVAILABLE_VOLUME_TEXTURES textureID);

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// Array of available internal (procedurally generated) two-dimensional textures
		// (UI sprites, the screen texture, etc)
		AthruTexture2D availableInternalTextures2D[(byteUnsigned)AVAILABLE_DISPLAY_TEXTURES::NULL_TEXTURE];

		// Array of available internal (procedurally generated) three-dimensional textures
		// (scene textures + abstract world textures (e.g. planetary delta-textures))
		AthruTexture3D availableInternalTextures3D[(byteUnsigned)AVAILABLE_VOLUME_TEXTURES::NULL_TEXTURE];
};

