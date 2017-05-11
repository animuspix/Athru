#pragma once

#include "Shader.h"
#include "AthruGlobals.h"
#include "RenderManager.h"

#define AMBIENT_DIFFUSE_RATIO 0.4f
#define SPOT_CUTOFF_RADIANS MathsStuff::PI / 8

class Rasterizer : public Shader
{
	public:
		Rasterizer(ID3D11Device* device, HWND windowHandle,
				   LPCWSTR vertexShaderFilePath, LPCWSTR pixelShaderFilePath);
		~Rasterizer();

		void SetShaderParameters(ID3D11DeviceContext * deviceContext, DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);
		void RenderShader(ID3D11DeviceContext * deviceContext);

		// Force render calls to go through here
		void Render(ID3D11DeviceContext* deviceContext,
					DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection,
					ID3D11ShaderResourceView* sceneColorTexture);

	private:
		struct LightBuffer
		{
			// Adding your own padding messes up DirectX's mapping between
			// CPU-side structs and GPU-side HLSL cbuffers, so just store
			// every element within 16-byte aligned types instead :P
			DirectX::XMFLOAT4 dirIntensity;
			DirectX::XMFLOAT4 dirDirection;
			DirectX::XMFLOAT4 dirDiffuse;
			DirectX::XMFLOAT4 dirAmbient;
			DirectX::XMFLOAT4 dirPos;

			DirectX::XMFLOAT4 pointIntensity[MAX_POINT_LIGHT_COUNT];
			DirectX::XMFLOAT4 pointDiffuse[MAX_POINT_LIGHT_COUNT];
			DirectX::XMFLOAT4 pointPos[MAX_POINT_LIGHT_COUNT];
			DirectX::XMUINT4 numPointLights;

			DirectX::XMFLOAT4 spotIntensity[MAX_SPOT_LIGHT_COUNT];
			DirectX::XMFLOAT4 spotDiffuse[MAX_SPOT_LIGHT_COUNT];
			DirectX::XMFLOAT4 spotPos[MAX_SPOT_LIGHT_COUNT];
			DirectX::XMFLOAT4 spotDirection[MAX_SPOT_LIGHT_COUNT];
			DirectX::XMFLOAT4 spotCutoffRadians;
			DirectX::XMUINT4 numSpotLights;

			DirectX::XMMATRIX worldMat;
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
		// one nicely simplifies the math for diffuse/specular PBR) (used by the pixel shader)
		ID3D11Buffer* lightBufferPttr;
};

