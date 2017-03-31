#include <assert.h>
#include "ServiceCentre.h"
#include "WICTextureLoader\WICTextureLoader.h"
#include "TextureManager.h"

// I decided that (since we're really only likely to use textures for
// shadowcasting + quick planet renders) building our own texture
// loader wasn't worth the time and effort needed. As a result, Athru
// will read and write all of it's textures through the DirectXTex
// library, which is (C) Microsoft 2017 and available over here:
// https://github.com/Microsoft/DirectXTex
//
// --
// ...And is distributed with the MIT license, which I've reprinted
// below in full:
//
// --------------------------------------------------------------------------------------|
//																						 |
//                                The MIT License (MIT)									 |
//																						 |
// Copyright (c) 2017 Microsoft Corp													 |
//																						 |
// Permission is hereby granted, free of charge, to any person obtaining a copy of this	 |
// software and associated documentation files (the "Software"), to deal in the Software |
// without restriction, including without limitation the rights to use, copy, modify,	 |
// merge, publish, distribute, sublicense, and/or sell copies of the Software, and to	 |
// permit persons to whom the Software is furnished to do so, subject to the following	 |
// conditions:																			 |
//																						 |
// The above copyright notice and this permission notice shall be included in all copies |
// or substantial portions of the Software.												 |
//																						 |
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,	 |
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A		 |
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT	 |
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF	 |
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE	 |
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         |
//																						 |
//---------------------------------------------------------------------------------------|

TextureManager::TextureManager(ID3D11Device* d3dDevice)
{
	// Long integer used for storing success/failure for different
	// DirectX operations
	HRESULT result;

	// DirectXTex texture loading is threaded, so call [CoInitializeEx]
	// here
	result = CoInitializeEx(nullptr, NULL);
	assert(SUCCEEDED(result));

	// Store texture locations
	textureLocations[(byteUnsigned)AVAILABLE_TEXTURES::BLANK_WHITE] = L"baseTex.bmp";
	textureLocations[(byteUnsigned)AVAILABLE_TEXTURES::CUBE_NORMAL] = L"normMapFront.bmp";

	// Build textures
	byteUnsigned i = 0;
	while (i < (byteUnsigned)AVAILABLE_TEXTURES::NULL_TEXTURE)
	{
		result = DirectX::CreateWICTextureFromFile(d3dDevice, textureLocations[i],
												   (ID3D11Resource**)(&(availableTextures[i].raw)), &(availableTextures[i].asShaderResource),
												   MAX_TEXTURE_SIZE);

		assert(SUCCEEDED(result));
		i += 1;
	}
}

TextureManager::~TextureManager()
{
	byteUnsigned i = 0;
	while (i < (byteUnsigned)AVAILABLE_TEXTURES::NULL_TEXTURE)
	{
		availableTextures[i].raw->Release();
		availableTextures[i].asShaderResource->Release();
		i += 1;
	}
}

AthruTexture& TextureManager::GetTexture(AVAILABLE_TEXTURES textureID)
{
	return availableTextures[(byteUnsigned)textureID];
}

// Push constructions for this class through Athru's custom allocator
void* TextureManager::operator new(size_t size)
{
	StackAllocator* allocator = ServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<TextureManager>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void TextureManager::operator delete(void* target)
{
	return;
}