#pragma once

#include <d3d11.h>
#include <directxmath.h>
#include <d3dcompiler.h>

#include "Matrix4.h"
#include "Logger.h"

enum class AVAILABLE_SHADERS
{
	VERT_PLOTTER,
	COLORIZER,
};

class ShaderCentre
{
	public:
		ShaderCentre(ID3D11Device* device, HWND windowHandle, Logger* logger);
		~ShaderCentre();

		void Render(ID3D11DeviceContext* deviceContext, 
					DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX projection);

		// Add functions allowing materials to subscribe to each supported shader
		// void SubscribeToVertPlotter();
		// void SubscribeToColorizer();

	private:
		struct Spatializer
		{
			DirectX::XMMATRIX world;
			DirectX::XMMATRIX view;
			DirectX::XMMATRIX projection;
		};

		// The base shader used to place Boxecules in-game
		ID3D11VertexShader* vertPlotter;

		// Add an array of materials subscribed to [vertPlotter] here
		// Add a tracker for the number of materials subscribed to [vertPlotter] here

		// The base shader used to color Boxecules in-game
		ID3D11PixelShader* colorizer;

		// Add an array of materials subscribed to [colorizer] here
		// Add a tracker for the number of materials subscribed to [colorizer] here

		// Render the materials subscribed to [vertPlotter], the materials subscribed to
		// [colorizer], and so on in each render pass

		// How vertices going into the GPU are set out
		ID3D11InputLayout* inputLayout;

		// A buffer used to store the world, view, and projection matrices
		ID3D11Buffer* matrixBuffer;
};

