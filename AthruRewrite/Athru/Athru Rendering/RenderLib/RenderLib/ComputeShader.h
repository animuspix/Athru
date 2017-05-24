#pragma once

#include <d3d11.h>
#include <windows.h>

class ComputeShader
{
	public:
		ComputeShader(ID3D11Device* device, HWND windowHandle,
					  LPCWSTR shaderFilePath);
		~ComputeShader();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	protected:
		// Helper function, used to convert a given position into a sampleable point within
		// the voxel grid
		DirectX::XMFLOAT4 ClipToViewableSpace(DirectX::XMVECTOR ssePosition);

		// The core shader object associated with [this]
		ID3D11ComputeShader* shader;
};
