#pragma once

#include "ComputeShader.h"

class DemoPlanetSampler : public ComputeShader
{
	public:
		DemoPlanetSampler(ID3D11Device* device, HWND windowHandle, 
						  LPCWSTR shaderFilePath);
		~DemoPlanetSampler();

		void Dispatch(ID3D11DeviceContext* context);

	private:
		// Compute-shader-friendly view of the scene color texture
		ID3D11ShaderResourceView* sceneColorTexture;

		// Compute-shader-friendly view of the scene normal texture
		ID3D11ShaderResourceView* sceneNormalTexture;

		// Compute-shader-friendly view of the scene PBR texture
		ID3D11ShaderResourceView* scenePBRTexture;

		// Compute-shader-friendly view of the scene emissivity texture
		ID3D11ShaderResourceView* sceneEmissivityTexture;
};

