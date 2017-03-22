#include "DepthOfField.h"

DepthOfField::DepthOfField(ID3D11Device* device, HWND windowHandle,
						   LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath) :
						   Shader(device, windowHandle, vertexShaderFilePath, pixelShaderFilePath)
{
}

DepthOfField::~DepthOfField()
{
	// Call the parent destructor
	Shader::~Shader();
}
