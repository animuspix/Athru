#pragma once

#include <d3d11.h>
#include <wrl\client.h>

class UpdateShader;
class GPUUpdateManager
{
	public:
		GPUUpdateManager();
		~GPUUpdateManager();

		// Perform GPU-side updates
		void Update();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	private:
		//Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3dContext;
		//UpdateShader* physics;
		//UpdateShader* cellDiff;
		//UpdateShader* ecology;
};
