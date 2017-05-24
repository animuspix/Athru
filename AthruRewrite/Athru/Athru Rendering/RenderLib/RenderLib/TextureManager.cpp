#include <assert.h>
#include "WICTextureLoader\WICTextureLoader.h"
#include "UtilityServiceCentre.h"
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

	// Build planetary heightfields

	// DirectXTex texture loading is threaded, so call [CoInitializeEx]
	// here
	result = CoInitializeEx(nullptr, NULL);
	assert(SUCCEEDED(result));

	// Store heightfield locations
	LPCWSTR planetaryHeightfieldLocations[2 * (byteUnsigned)AVAILABLE_PLANETARY_HEIGHTFIELDS::NULL_TEXTURE];
	planetaryHeightfieldLocations[(byteUnsigned)AVAILABLE_PLANETARY_HEIGHTFIELDS::ARCHETYPE_ZERO] = L"tectoUpperZero.bmp";
	planetaryHeightfieldLocations[(byteUnsigned)AVAILABLE_PLANETARY_HEIGHTFIELDS::ARCHETYPE_ZERO + 1] = L"tectoLowerZero.bmp";

	// Build compute-friendly heightfield buffer description
	// Note: All heightfields are expected to be 512 pixels wide, 512 pixels tall,
	// and saved in the .bmp file format
	D3D11_BUFFER_DESC heightfieldBufferDesc;
	heightfieldBufferDesc.ByteWidth = (fourByteUnsigned)((sizeof(float) * 4) * GraphicsStuff::HEIGHTFIELD_AREA);
	heightfieldBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	heightfieldBufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	heightfieldBufferDesc.StructureByteStride = sizeof(float) * 4;
	heightfieldBufferDesc.CPUAccessFlags = 0;
	heightfieldBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	for (byteUnsigned i = 0; i < (byteUnsigned)AVAILABLE_PLANETARY_HEIGHTFIELDS::NULL_TEXTURE; i += 1)
	{
		// Import resources for the upper/lower hemispheres of
		// the current heightfield

		result = DirectX::CreateWICTextureFromFile(d3dDevice, planetaryHeightfieldLocations[i],
												   (ID3D11Resource**)(&(availablePlanetaryHeightfields[i].upperHemisphere.raw)),
												   &(availablePlanetaryHeightfields[i].upperHemisphere.asRenderedShaderResource),
												   MAX_TEXTURE_SIZE_2D);
		assert(SUCCEEDED(result));

		result = DirectX::CreateWICTextureFromFile(d3dDevice, planetaryHeightfieldLocations[i],
												   (ID3D11Resource**)(&(availablePlanetaryHeightfields[i].lowerHemisphere.raw)),
												   &(availablePlanetaryHeightfields[i].lowerHemisphere.asRenderedShaderResource),
												   MAX_TEXTURE_SIZE_2D);
		assert(SUCCEEDED(result));

		// Build compute-friendly buffers with the raw texture data
		// generated above

		D3D11_SUBRESOURCE_DATA textureData;
		textureData.pSysMem = &(availableInternalTextures3D[i].raw);
		textureData.SysMemPitch = (fourByteUnsigned)GraphicsStuff::HEIGHTFIELD_WIDTH;
		textureData.SysMemSlicePitch = (fourByteUnsigned)GraphicsStuff::HEIGHTFIELD_AREA;

		result = d3dDevice->CreateBuffer(&heightfieldBufferDesc, &textureData, &(availablePlanetaryHeightfields[i].upperHemisphere.asStructuredBuffer));
		assert(SUCCEEDED(result));

		result = d3dDevice->CreateBuffer(&heightfieldBufferDesc, &textureData, &(availablePlanetaryHeightfields[i].lowerHemisphere.asStructuredBuffer));
		assert(SUCCEEDED(result));

		// Extract compute-friendly resource views from the buffers generated
		// above

		result = d3dDevice->CreateShaderResourceView(availablePlanetaryHeightfields[i].upperHemisphere.raw, nullptr,
													 &(availablePlanetaryHeightfields[i].upperHemisphere.asComputeShaderResource));
		assert(SUCCEEDED(result));

		result = d3dDevice->CreateShaderResourceView(availablePlanetaryHeightfields[i].lowerHemisphere.raw, nullptr,
													 &(availablePlanetaryHeightfields[i].lowerHemisphere.asComputeShaderResource));
		assert(SUCCEEDED(result));

		// Heightfields are strictly read-only, so avoid giving them a write-allowed resource view
		availablePlanetaryHeightfields[i].lowerHemisphere.asWritableComputeShaderResource = nullptr;
		availablePlanetaryHeightfields[i].upperHemisphere.asWritableComputeShaderResource = nullptr;

		i += 1;
	}

	// Build display textures

	// Create display texture description
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

	for (byteUnsigned i = 0; i < (byteUnsigned)AVAILABLE_DISPLAY_TEXTURES::NULL_TEXTURE; i += 1)
	{
		// If the current texture is an effect mask, adjust the texture format
		// to match
		if ((AVAILABLE_DISPLAY_TEXTURES)i != AVAILABLE_DISPLAY_TEXTURES::SCREEN_TEXTURE)
		{
			screenTextureDesc.Format = DXGI_FORMAT_R32_FLOAT;
		}

		// Build texture
		result = d3dDevice->CreateTexture2D(&screenTextureDesc, nullptr, &(availableInternalTextures2D[i].raw));
		assert(SUCCEEDED(result));

		// Extract a shader-friendly resource view from the generated texture
		result = d3dDevice->CreateShaderResourceView(availableInternalTextures2D[i].raw, nullptr, &(availableInternalTextures2D[i].asRenderedShaderResource));
		assert(SUCCEEDED(result));

		// Post-processing effects will never be compute-shaded, so just set compute-shading properties to [nullptr]
		availableInternalTextures2D[i].asStructuredBuffer = nullptr;
		availableInternalTextures2D[i].asComputeShaderResource = nullptr;
		availableInternalTextures2D[i].asWritableComputeShaderResource = nullptr;
	}

	// Build scene textures

	// Material properties stored in volume textures
	// - Color (including alpha)
	// - PBR (roughness/reflectance)
	// - Emissive intensity

	// Create scene texture description
	D3D11_TEXTURE3D_DESC sceneTextureDesc;
	sceneTextureDesc.Width = GraphicsStuff::VOXEL_GRID_WIDTH;
	sceneTextureDesc.Height = GraphicsStuff::VOXEL_GRID_WIDTH;
	sceneTextureDesc.Depth = GraphicsStuff::VOXEL_GRID_WIDTH;
	sceneTextureDesc.MipLevels = 1;
	sceneTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	sceneTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	sceneTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	sceneTextureDesc.CPUAccessFlags = 0;
	sceneTextureDesc.MiscFlags = 0;

	for (byteUnsigned i = 0; i < (byteUnsigned)AVAILABLE_VOLUME_TEXTURES::NULL_TEXTURE; i += 1)
	{
		// Set the number of active elements within each channel of the current texture
		byteUnsigned elementsPerChannel = 4;
		sceneTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		if ((AVAILABLE_VOLUME_TEXTURES)i == AVAILABLE_VOLUME_TEXTURES::SCENE_EMISSIVITY_TEXTURE)
		{
			elementsPerChannel = 1;
			sceneTextureDesc.Format = DXGI_FORMAT_R32_FLOAT;
		}

		else if ((AVAILABLE_VOLUME_TEXTURES)i == AVAILABLE_VOLUME_TEXTURES::SCENE_PBR_TEXTURE)
		{
			elementsPerChannel = 2;
			sceneTextureDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
		}

		// Set the width/height/depth of the current texture
		sceneTextureDesc.Width = GraphicsStuff::VOXEL_GRID_WIDTH;
		sceneTextureDesc.Height = GraphicsStuff::VOXEL_GRID_WIDTH;
		sceneTextureDesc.Depth = GraphicsStuff::VOXEL_GRID_WIDTH;
		if ((AVAILABLE_VOLUME_TEXTURES)i == AVAILABLE_VOLUME_TEXTURES::SCENE_NORMAL_TEXTURE ||
			(AVAILABLE_VOLUME_TEXTURES)i == AVAILABLE_VOLUME_TEXTURES::SCENE_EMISSIVITY_TEXTURE)
		{
			// Assume one normal per face + per-face emissivity
			sceneTextureDesc.Width = GraphicsStuff::VOXEL_GRID_WIDTH * 6;
			sceneTextureDesc.Height = GraphicsStuff::VOXEL_GRID_WIDTH * 6;
			sceneTextureDesc.Depth = GraphicsStuff::VOXEL_GRID_WIDTH * 6;
		}

		// Build texture
		result = d3dDevice->CreateTexture3D(&sceneTextureDesc, nullptr, &(availableInternalTextures3D[i].raw));
		assert(SUCCEEDED(result));

		// Extract a shader-friendly resource view from the generated texture
		result = d3dDevice->CreateShaderResourceView(availableInternalTextures3D[i].raw, nullptr, &(availableInternalTextures3D[i].asRenderedShaderResource));
		assert(SUCCEEDED(result));

		// Build compute-friendly texture buffer description
		D3D11_BUFFER_DESC sceneBufferDesc;
		sceneBufferDesc.ByteWidth = (fourByteUnsigned)((sizeof(float) * elementsPerChannel) * GraphicsStuff::VOXEL_GRID_VOLUME);
		sceneBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		sceneBufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		sceneBufferDesc.StructureByteStride = sizeof(float) * elementsPerChannel;
		sceneBufferDesc.CPUAccessFlags = 0;
		sceneBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

		if ((AVAILABLE_VOLUME_TEXTURES)i == AVAILABLE_VOLUME_TEXTURES::SCENE_NORMAL_TEXTURE ||
			(AVAILABLE_VOLUME_TEXTURES)i == AVAILABLE_VOLUME_TEXTURES::SCENE_EMISSIVITY_TEXTURE)
		{
			// Assume one normal per face + per-face emissivity
			sceneBufferDesc.ByteWidth = (fourByteUnsigned)((sizeof(float) * elementsPerChannel) * GraphicsStuff::VOXEL_GRID_VOLUME * 6);
			sceneBufferDesc.StructureByteStride = sizeof(float) * elementsPerChannel * 6;
		}

		// Build a compute-friendly buffer with the raw texture data
		D3D11_SUBRESOURCE_DATA textureData;
		textureData.pSysMem = &(availableInternalTextures3D[i].raw);
		textureData.SysMemPitch = (fourByteUnsigned)GraphicsStuff::VOXEL_GRID_WIDTH;
		textureData.SysMemSlicePitch = (fourByteUnsigned)GraphicsStuff::VOXEL_GRID_AREA;
		result = d3dDevice->CreateBuffer(&sceneBufferDesc, &textureData, &(availableInternalTextures3D[i].asStructuredBuffer));
		assert(SUCCEEDED(result));

		// Extract a compute-friendly resource view from the generated buffer
		result = d3dDevice->CreateShaderResourceView(availableInternalTextures3D[i].asStructuredBuffer, nullptr, &(availableInternalTextures3D[i].asComputeShaderResource));
		assert(SUCCEEDED(result));

		// Extract a writable compute-friendly shader resource view from the generated buffer
		// Unordered-access views can't be created for 3D texture resources, so
		result = d3dDevice->CreateUnorderedAccessView(availableInternalTextures3D[i].asStructuredBuffer, nullptr, &(availableInternalTextures3D[i].asWritableComputeShaderResource));
		assert(SUCCEEDED(result));
	}
}

TextureManager::~TextureManager()
{
	for (byteUnsigned i = 0; i < (byteUnsigned)AVAILABLE_PLANETARY_HEIGHTFIELDS::NULL_TEXTURE; i += 1)
	{
		availablePlanetaryHeightfields[i].ReleaseTextures();
		i += 1;
	}

	for (byteUnsigned i = 0; i < (byteUnsigned)AVAILABLE_DISPLAY_TEXTURES::NULL_TEXTURE; i += 1)
	{
		availableInternalTextures2D[i].raw->Release();
		availableInternalTextures2D[i].raw = nullptr;

		availableInternalTextures2D[i].asRenderedShaderResource->Release();
		availableInternalTextures2D[i].asRenderedShaderResource = nullptr;

		// Structured buffers/compute-shader resources are kept at [nullptr]
		// for display textures, so no reason to release them here

		i += 1;
	}

	for (byteUnsigned i = 0; i < (byteUnsigned)AVAILABLE_VOLUME_TEXTURES::NULL_TEXTURE; i += 1)
	{
		availableInternalTextures3D[i].raw->Release();
		availableInternalTextures3D[i].raw = nullptr;

		availableInternalTextures3D[i].asRenderedShaderResource->Release();
		availableInternalTextures3D[i].asRenderedShaderResource = nullptr;

		availableInternalTextures3D[i].asStructuredBuffer->Release();
		availableInternalTextures3D[i].asStructuredBuffer = nullptr;

		availableInternalTextures3D[i].asComputeShaderResource->Release();
		availableInternalTextures3D[i].asComputeShaderResource = nullptr;

		i += 1;
	}
}

Heightfield& TextureManager::GetPlanetaryHeightfield(AVAILABLE_PLANETARY_HEIGHTFIELDS textureSetID)
{
	return availablePlanetaryHeightfields[(byteUnsigned)textureSetID];
}

AthruTexture2D& TextureManager::GetDisplayTexture(AVAILABLE_DISPLAY_TEXTURES textureID)
{
	return availableInternalTextures2D[(byteUnsigned)textureID];
}

AthruTexture3D& TextureManager::GetVolumeTexture(AVAILABLE_VOLUME_TEXTURES textureID)
{
	return availableInternalTextures3D[(byteUnsigned)textureID];
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