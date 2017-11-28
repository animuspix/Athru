#pragma once

#include "Direct3D.h"
#include "UpdateShader.h"

class GPUUpdateManager
{
	public:
		GPUUpdateManager();
		~GPUUpdateManager();

		// Perform GPU-side updates
		void Update();

	private:
		ID3D11DeviceContext* d3dContext;
		UpdateShader* physics;
		UpdateShader* cellDiff;
		UpdateShader* ecology;
};

