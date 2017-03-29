#include "SpotLight.h"

SpotLight::SpotLight(ID3D11Device* device, HWND windowHandle,
				     LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath) :
					 Shader(device, windowHandle, vertexShaderFilePath, pixelShaderFilePath)
{
}

SpotLight::~SpotLight()
{
}
