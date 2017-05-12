#pragma once

#define SCREEN_RECT_INDEX_COUNT 6

#include "Mesh.h"
#include "AthruTexture2D.h"

class ScreenRect : public Mesh
{
	public:
		ScreenRect(ID3D11Device* d3dDevice);
		~ScreenRect();

		// Retrieve the render-target-view associated with [this]
		ID3D11RenderTargetView* GetRenderTarget();

	private:
		// The texture painted onto [this]
		AthruTexture2D texture;

		// The render-target-view used to dynamically modify
		// the texture associated with [this]
		ID3D11RenderTargetView* renderTarget;
};

