#include "PointLight.h"

PointLight::PointLight(ID3D11Device* device, HWND windowHandle,
					   LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath) :
					   Shader(device, windowHandle, vertexShaderFilePath, pixelShaderFilePath)
{
}

PointLight::~PointLight()
{
}
