#include "UtilityServiceCentre.h"
#include "AthruGlobals.h"
#include "PostVertex.h"
#include "ViewFinder.h"
#include "AthruRect.h"

ViewFinder::ViewFinder(const Microsoft::WRL::ComPtr<ID3D11Device>& device)
{
	// Construct an [AthruRect] primitive
	AthruRect<PostVertex> dispRect = AthruRect<PostVertex>();

	// Cosntruct [viewGeo] with the vertices defined for [dispRect]
	viewGeo = Mesh<PostVertex>(dispRect.vts,
							   dispRect.indices,
							   4,
							   GraphicsStuff::SCREEN_RECT_INDEX_COUNT,
							   device);

	// Describe the display texture
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

	// Create the display texture
	HRESULT result = device->CreateTexture2D(&screenTextureDesc, nullptr, &(displayTex.raw));
	assert(SUCCEEDED(result));
	result = device->CreateShaderResourceView(displayTex.raw.Get(), nullptr, &(displayTex.asReadOnlyShaderResource));
	assert(SUCCEEDED(result));
	result = device->CreateUnorderedAccessView(displayTex.raw.Get(), nullptr, &(displayTex.asWritableShaderResource));
	assert(SUCCEEDED(result));
}

ViewFinder::~ViewFinder() {}

Mesh<PostVertex>& ViewFinder::GetViewGeo()
{
	return viewGeo;
}

AthruTexture2D& ViewFinder::GetDisplayTex()
{
	return displayTex;
}

// Push constructions for this class through Athru's custom allocator
void* ViewFinder::operator new(size_t size)
{
	StackAllocator* allocator = AthruUtilities::UtilityServiceCentre::AccessMemory();
	return allocator->AlignedAlloc(size, (byteUnsigned)std::alignment_of<ViewFinder>(), false);
}

// We aren't expecting to use [delete], so overload it to do nothing
void ViewFinder::operator delete(void* target)
{
	return;
}
