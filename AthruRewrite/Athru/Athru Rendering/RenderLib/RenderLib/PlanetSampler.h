#pragma once

#include "ComputeShader.h"

class PlanetSampler : public ComputeShader
{
	public:
		PlanetSampler(ID3D11Device* device, HWND windowHandle,
					  LPCWSTR shaderFilePath,
					  ID3D11ShaderResourceView* upperHeightfield,
					  ID3D11ShaderResourceView* lowerHeightfield,
					  ID3D11UnorderedAccessView* writableSceneColorResource,
					  ID3D11UnorderedAccessView* sceneNormalShaderResource,
					  ID3D11UnorderedAccessView* scenePBRShaderResource,
					  ID3D11UnorderedAccessView* sceneEmissivityShaderResource);

		~PlanetSampler();

		void Dispatch(ID3D11DeviceContext* context, DirectX::XMVECTOR planetPosition,
					  DirectX::XMVECTOR cameraPosition, DirectX::XMFLOAT4 avgPlanetColor,
					  DirectX::XMVECTOR starPosition);

	private:
		// Planetary sampling data
		// Simplify the sampling process by ignoring weighting
		// for now
		struct PlanetBuffer
		{
			//DirectX::XMFLOAT4 weighting;
			DirectX::XMFLOAT4 cameraSurfacePos;
			DirectX::XMFLOAT4 planetAvgColor;
			DirectX::XMFLOAT4 starPos;
		};

		// Read-only compute-shader-friendly view of the upper-hemisphere heightfield
		ID3D11ShaderResourceView* upperTectoTexture;

		// Read-only compute-shader-friendly view of the lower-hemisphere heightfield
		ID3D11ShaderResourceView* lowerTectoTexture;

		// Writable compute-shader-friendly view of the scene color texture
		ID3D11UnorderedAccessView* sceneColorTexture;

		// Read-only compute-shader-friendly view of the scene normal texture
		ID3D11UnorderedAccessView* sceneNormalTexture;

		// Read-only compute-shader-friendly view of the scene PBR texture
		ID3D11UnorderedAccessView* scenePBRTexture;

		// Read-only compute-shader-friendly view of the scene emissivity texture
		ID3D11UnorderedAccessView* sceneEmissivityTexture;

		// Per-dispatch storage for planetary sampling data
		ID3D11Buffer* planetBufferPttr;
};

