#include "CookTorrancePBR.h"

CookTorrancePBR::CookTorrancePBR(ID3D11Device* device, HWND windowHandle,
								 LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath) :
								 Shader(device, windowHandle, vertexShaderFilePath, pixelShaderFilePath)
{
}

CookTorrancePBR::~CookTorrancePBR()
{
	// Call the parent destructor
	Shader::~Shader();
}
