#include <assert.h>
#include "UtilityServiceCentre.h"
#include "TextureManager.h"

TextureManager::TextureManager(ID3D11Device* d3dDevice)
{
	// Long integer used for storing success/failure for different
	// DirectX operations
	HRESULT result;

	// Build display texture

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
	screenTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	screenTextureDesc.CPUAccessFlags = 0;
	screenTextureDesc.MiscFlags = 0;

	// Build texture
	result = d3dDevice->CreateTexture2D(&screenTextureDesc, nullptr, &(availableInternalTextures2D[0].raw));
	assert(SUCCEEDED(result));

	// Extract a read-only shader-friendly resource view from the generated texture
	result = d3dDevice->CreateShaderResourceView(availableInternalTextures2D[0].raw, nullptr, &(availableInternalTextures2D[0].asReadOnlyShaderResource));
	assert(SUCCEEDED(result));

	// Extract a writable shader resource view from the generated texture
	result = d3dDevice->CreateUnorderedAccessView(availableInternalTextures2D[0].raw, nullptr, &(availableInternalTextures2D[0].asWritableShaderResource));
	assert(SUCCEEDED(result));
}

TextureManager::~TextureManager()
{
	for (byteUnsigned i = 0; i < (byteUnsigned)AVAILABLE_DISPLAY_TEXTURES::NULL_TEXTURE; i += 1)
	{
		availableInternalTextures2D[i].raw->Release();
		availableInternalTextures2D[i].raw = nullptr;

		availableInternalTextures2D[i].asReadOnlyShaderResource->Release();
		availableInternalTextures2D[i].asReadOnlyShaderResource = nullptr;

		availableInternalTextures2D[i].asWritableShaderResource->Release();
		availableInternalTextures2D[i].asWritableShaderResource = nullptr;
	}
}

AthruTexture2D& TextureManager::GetDisplayTexture(AVAILABLE_DISPLAY_TEXTURES textureID)
{
	return availableInternalTextures2D[(byteUnsigned)textureID];
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