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

	private:
		ID3D11ComputeShader* shader;
};
