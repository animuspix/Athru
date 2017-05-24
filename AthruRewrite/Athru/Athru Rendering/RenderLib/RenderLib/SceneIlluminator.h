#pragma once

#include "ComputeShader.h"

class SceneIlluminator : public ComputeShader
{
	public:
		SceneIlluminator(ID3D11Device* device, HWND windowHandle,
					     LPCWSTR shaderFilePath,
						 ID3D11UnorderedAccessView* writableColorShaderResource,
					     ID3D11ShaderResourceView* sceneNormalShaderResource,
					     ID3D11ShaderResourceView* scenePBRShaderResource,
					     ID3D11ShaderResourceView* sceneEmissivityShaderResource);

		~SceneIlluminator();

		void Dispatch(ID3D11DeviceContext* context,
					  DirectX::XMVECTOR cameraViewVector);

	private:
		// Per-dispatch camera/view data needed for PBR-style
		// scene lighting
		struct CameraDataBuffer
		{
			// The camera's local view vector (normalized)
			DirectX::XMFLOAT4 cameraViewVec;
		};

		// Compute-shader-friendly read/write allowed representation
		// of the scene color texture
		ID3D11UnorderedAccessView* sceneColorTexture;

		// Compute-shader-friendly view of the scene normal texture
		ID3D11ShaderResourceView* sceneNormalTexture;

		// Compute-shader-friendly view of the scene PBR texture
		ID3D11ShaderResourceView* scenePBRTexture;

		// Compute-shader-friendly view of the scene emissivity texture
		ID3D11ShaderResourceView* sceneEmissivityTexture;

		// Per-dispatch storage for lighting-relevant camera data
		ID3D11Buffer* cameraDataBufferPttr;
};

