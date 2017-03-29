#pragma once

#include "Shader.h"
#include "MathIncludes.h"

#define AMBIENT_DIFFUSE_RATIO 0.4f
#define SPOT_CUTOFF_RADIANS PI / 2

class Rasterizer : public Shader
{
	public:
		Rasterizer(ID3D11Device* device, HWND windowHandle,
				   LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath);
		~Rasterizer();

		// Force render calls to go through here
		void Render(ID3D11DeviceContext* deviceContext,
					float dirLightIntensity, DirectX::XMFLOAT4 dirLightDirection,
					DirectX::XMFLOAT4 dirLightDiffuse, DirectX::XMFLOAT4 dirLightAmbient,
					DirectX::XMFLOAT4 dirLightPos,
					float* pointLightIntensities, DirectX::XMFLOAT4* pointLightDiffuseColors,
					DirectX::XMFLOAT4* pointLightPositions, fourByteUnsigned numPointLights,
					float* spotLightIntensities, DirectX::XMFLOAT4* spotLightDiffuseColors,
					DirectX::XMFLOAT4* spotLightPositions, DirectX::XMFLOAT4* spotLightDirections,
					fourByteUnsigned numSpotLights,
					DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection,
					ID3D11ShaderResourceView* texture, fourByteUnsigned numIndicesDrawing);

	private:
		struct LightBuffer
		{
			// Lots of padding here to maintain 16-byte alignment
			float dirIntensity;
			DirectX::XMFLOAT3 dirIntensityPadding;

			DirectX::XMFLOAT4 dirDirection;
			DirectX::XMFLOAT4 dirDiffuse;
			DirectX::XMFLOAT4 dirAmbient;
			DirectX::XMFLOAT4 dirPos;

			float pointIntensity[MAX_POINT_LIGHT_COUNT];
			DirectX::XMFLOAT3 pointIntensityPadding[MAX_POINT_LIGHT_COUNT];

			DirectX::XMFLOAT4 pointDiffuse[MAX_POINT_LIGHT_COUNT];
			DirectX::XMFLOAT4 pointPos[MAX_POINT_LIGHT_COUNT];

			fourByteUnsigned numPointLights;
			DirectX::XMFLOAT3 numPointLightPadding;

			float spotIntensity[MAX_SPOT_LIGHT_COUNT];
			DirectX::XMFLOAT3 spotIntensityPadding[MAX_SPOT_LIGHT_COUNT];

			DirectX::XMFLOAT4 spotDiffuse[MAX_SPOT_LIGHT_COUNT];
			DirectX::XMFLOAT4 spotPos[MAX_SPOT_LIGHT_COUNT];
			DirectX::XMFLOAT4 spotDirection[MAX_SPOT_LIGHT_COUNT];

			float spotCutoffRadians;
			DirectX::XMFLOAT3 spotCutoffRadiansPadding;

			fourByteUnsigned numSpotLights;
			DirectX::XMFLOAT3 numSpotLightPadding;

			DirectX::XMFLOAT4 viewVec;
		};

		// Overload ordinary external shader render to do nothing
		// Also place the overload in [private] so it becomes inaccessible
		void Render(ID3D11DeviceContext* deviceContext,
					DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection,
					fourByteUnsigned numIndicesDrawing) {};

		// The state of the texture sampler used by [this]
		ID3D11SamplerState* wrapSamplerState;

		// Per-frame storage for light properties (intensity, direction, diffuse/ambient tint,
		// and position; strictly speaking directional lights don't have a positional component, but adding
		// one heavily simplifies the math for diffuse/specular PBR)
		ID3D11Buffer* lightBufferPttr;
};

