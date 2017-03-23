#include "BufferRasterizer.h"

BufferRasterizer::BufferRasterizer(ID3D11Device* device, HWND windowHandle,
								   LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath) :
								   Shader(device, windowHandle, vertexShaderFilePath, pixelShaderFilePath) {}

BufferRasterizer::~BufferRasterizer()
{
	// Call the parent destructor
	Shader::~Shader();
}
