#include "Rasterizer.h"

Rasterizer::Rasterizer(ID3D11Device* device, HWND windowHandle,
					   LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath) :
					   Shader(device, windowHandle, vertexShaderFilePath, pixelShaderFilePath)
{
}

Rasterizer::~Rasterizer()
{
}
