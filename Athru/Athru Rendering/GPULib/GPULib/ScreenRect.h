#pragma once

#include "Mesh.h"
#include "AthruTexture2D.h"

class ScreenRect : public Mesh
{
	public:
		ScreenRect(ID3D11Device* d3dDevice);
		~ScreenRect();

		// Pass [this] onto the GPU for rendering
		void PassToGPU(ID3D11DeviceContext* context);
};

