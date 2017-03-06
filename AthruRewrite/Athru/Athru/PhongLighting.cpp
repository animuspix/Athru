#include "PhongLighting.h"

PhongLighting::PhongLighting(ID3D11Device* device, HWND windowHandle,
							 LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath) :
						     Shader(device, windowHandle, vertexShaderFilePath, pixelShaderFilePath)
{

}

PhongLighting::~PhongLighting()
{
}

void PhongLighting::SetShaderParameters(ID3D11DeviceContext* deviceContext,
										DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection,
										DirectX::XMFLOAT4 ambientColor, DirectX::XMFLOAT4 solarColor)
{
	Shader::SetShaderParameters(deviceContext, world, view, projection);

	// Set lighting parameters here...
}
