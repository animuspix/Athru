#pragma once

#include "Mesh.h"
#include "AthruTexture2D.h"
#include "PostVertex.h"

class ViewFinder
{
	public:
		ViewFinder(const Microsoft::WRL::ComPtr<ID3D11Device>& device);
		~ViewFinder();

		// Access viewfinder's pipeline geometry
		Mesh<PostVertex>& GetViewGeo();

		// Access the display texture
		AthruTexture2D& GetDisplayTex();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		// Pipeline geometry associated with [this]
		Mesh<PostVertex> viewGeo;

		// Display texture associated with [this]
		AthruTexture2D displayTex;
};

