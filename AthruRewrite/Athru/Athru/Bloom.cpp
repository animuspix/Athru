#include "Bloom.h"

Bloom::Bloom(ID3D11Device* device, HWND windowHandle,
			 LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath) :
			 Shader(device, windowHandle, vertexShaderFilePath, pixelShaderFilePath)
{
}

Bloom::~Bloom()
{
	// Call the parent destructor
	Shader::~Shader();
}
