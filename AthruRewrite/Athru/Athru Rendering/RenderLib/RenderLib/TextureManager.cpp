#include <assert.h>
#include "stdafx.h"
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
	textureLocations[(byteUnsigned)AVAILABLE_EXTERNAL_TEXTURES::BLANK_WHITE] = L"baseTex.bmp";
	textureLocations[(byteUnsigned)AVAILABLE_EXTERNAL_TEXTURES::IMPORTED_MESH_TEXTURE] = L"importedMeshTexture.bmp";

	// Build external textures
	byteUnsigned i = 0;
	while (i < (byteUnsigned)AVAILABLE_EXTERNAL_TEXTURES::NULL_TEXTURE)
	{
		result = DirectX::CreateWICTextureFromFile(d3dDevice, textureLocations[i],
												   (ID3D11Resource**)(&(availableExternalTextures[i].raw)), &(availableExternalTextures[i].asShaderResource),
												   MAX_TEXTURE_SIZE_2D);

		assert(SUCCEEDED(result));
		i += 1;
	}

	// Build internal textures

	// Create post-processing texture description
	D3D11_TEXTURE2D_DESC screenTextureDesc;
	screenTextureDesc.Width = GraphicsStuff::DISPLAY_WIDTH;
	screenTextureDesc.Height = GraphicsStuff::DISPLAY_HEIGHT;
	screenTextureDesc.MipLevels = 1;
	screenTextureDesc.ArraySize = 1;
	screenTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	screenTextureDesc.SampleDesc.Count = 1;
	screenTextureDesc.SampleDesc.Quality = 0;
	screenTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	screenTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	screenTextureDesc.CPUAccessFlags = 0;
	screenTextureDesc.MiscFlags = 0;

	// Build raw post-processing texture
	byteUnsigned screenTextureID = (byteUnsigned)AVAILABLE_INTERNAL_TEXTURES::SCREEN_TEXTURE;
	result = d3dDevice->CreateTexture2D(&screenTextureDesc, nullptr, &(availableInternalTextures[screenTextureID].raw));
	assert(SUCCEEDED(result));

	// Extract a shader-friendly resource view from the generated texture
	result = d3dDevice->CreateShaderResourceView(availableInternalTextures[screenTextureID].raw, nullptr, &(availableInternalTextures[screenTextureID].asShaderResource));
	assert(SUCCEEDED(result));
}

TextureManager::~TextureManager()
{
	byteUnsigned i = 0;
	while (i < (byteUnsigned)AVAILABLE_EXTERNAL_TEXTURES::NULL_TEXTURE)
	{
		availableExternalTextures[i].raw->Release();
		availableExternalTextures[i].raw = nullptr;

		availableExternalTextures[i].asShaderResource->Release();
		availableExternalTextures[i].asShaderResource = nullptr;
		i += 1;
	}

	byteUnsigned j = 0;
	while (j < (byteUnsigned)AVAILABLE_INTERNAL_TEXTURES::NULL_TEXTURE)
	{
		availableInternalTextures[j].raw->Release();
		availableInternalTextures[j].raw = nullptr;

		availableInternalTextures[j].asShaderResource->Release();
		availableInternalTextures[j].asShaderResource = nullptr;

		j += 1;
	}
}

AthruTexture2D& TextureManager::GetExternalTexture2D(AVAILABLE_EXTERNAL_TEXTURES textureID)
{
	return availableExternalTextures[(byteUnsigned)textureID];
}

AthruTexture2D& TextureManager::GetInternalTexture2D(AVAILABLE_INTERNAL_TEXTURES textureID)
{
	return availableInternalTextures[(byteUnsigned)textureID];
}

// Push constructions for this class through Athru's custom allocator
void* TextureManager::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<TextureManager>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void TextureManager::operator delete(void* target)
{
	return;
}