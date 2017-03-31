#pragma once

#include "Typedefs.h"
#include "d3d11.h"
#include "AthruTexture.h"

#define MAX_TEXTURE_SIZE (32684 * 32684)

enum class AVAILABLE_TEXTURES
{
	BLANK_WHITE,
	CUBE_NORMAL,
	NULL_TEXTURE
};

class TextureManager
{
	public:
		TextureManager(ID3D11Device* d3dDevice);
		~TextureManager();

		AthruTexture& GetTexture(AVAILABLE_TEXTURES textureID);

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		AthruTexture availableTextures[(byteUnsigned)AVAILABLE_TEXTURES::NULL_TEXTURE];
		LPCWSTR textureLocations[(byteUnsigned)AVAILABLE_TEXTURES::NULL_TEXTURE];
};

