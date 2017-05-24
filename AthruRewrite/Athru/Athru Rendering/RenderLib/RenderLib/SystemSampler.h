#pragma once

#include "ComputeShader.h"

class SystemSampler : public ComputeShader
{
	public:
		SystemSampler(ID3D11Device* device, HWND windowHandle,
					  LPCWSTR shaderFilePath,
					  ID3D11UnorderedAccessView* writableSceneColorResource,
					  ID3D11UnorderedAccessView* sceneNormalShaderResource,
					  ID3D11UnorderedAccessView* scenePBRShaderResource,
					  ID3D11UnorderedAccessView* sceneEmissivityShaderResource);

		~SystemSampler();

		void Dispatch(ID3D11DeviceContext* context,
					  DirectX::XMVECTOR cameraPosition, DirectX::XMVECTOR starPosition,
					  DirectX::XMFLOAT4 localStarColor, DirectX::XMVECTOR planetAPosition,
					  DirectX::XMVECTOR planetBPosition, DirectX::XMVECTOR planetCPosition,
					  DirectX::XMFLOAT4 planetAColor, DirectX::XMFLOAT4 planetBColor,
					  DirectX::XMFLOAT4 planetCColor);

	private:
		// Per-thread system sampling data
		struct SystemBuffer
		{
			DirectX::XMFLOAT4 cameraPos;

			DirectX::XMFLOAT4 starPos;
			DirectX::XMFLOAT4 starColor;

			DirectX::XMFLOAT4 planetAPos;
			DirectX::XMFLOAT4 planetBPos;
			DirectX::XMFLOAT4 planetCPos;

			DirectX::XMFLOAT4 planetAColor;
			DirectX::XMFLOAT4 planetBColor;
			DirectX::XMFLOAT4 planetCColor;
		};

		// Writable compute-shader-friendly view of the scene color texture
		ID3D11UnorderedAccessView* sceneColorTexture;

		// Read-only compute-shader-friendly view of the scene normal texture
		ID3D11UnorderedAccessView* sceneNormalTexture;

		// Read-only compute-shader-friendly view of the scene PBR texture
		ID3D11UnorderedAccessView* scenePBRTexture;

		// Read-only compute-shader-friendly view of the scene emissivity texture
		ID3D11UnorderedAccessView* sceneEmissivityTexture;

		// Per-thread storage for system sampling data
		ID3D11Buffer* systemBufferPttr;
};
