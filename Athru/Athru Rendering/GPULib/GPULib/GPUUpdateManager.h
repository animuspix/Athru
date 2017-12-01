#pragma once

#include <d3d11.h>

class UpdateShader;
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

