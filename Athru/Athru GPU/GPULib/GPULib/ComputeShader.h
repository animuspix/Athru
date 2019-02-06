#pragma once

#include <d3d12.h>
#include <windows.h>
#include <wrl\client.h>

class ComputeShader
{
	public:
		ComputeShader(const Microsoft::WRL::ComPtr<ID3D11Device>& device,
					  HWND windowHandle,
					  LPCWSTR shaderFilePath);
		~ComputeShader();

		// Overload the standard allocation/de-allocation operators
		void* operator new(size_t size);
		void operator delete(void* target);

	public:
		// The core shader object associated with [this]
		Microsoft::WRL::ComPtr<ID3D11ComputeShader> shader;
};
