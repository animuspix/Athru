#pragma once

// Alternative way to do this?
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")

#include "dxgi.h"
#include "d3dcommon.h"
#include "d3d11.h"

#include "Matrix4.h"

class Direct3D
{
	public:
		// Ask Adam how I should resolve this; ignore for now
		Direct3D();
		Direct3D(unsigned int screenWidth, unsigned int screenHeight, bool vsyncActive, HWND hwnd, bool isFullscreen,
				 float screenFullDepth, float screenNearDepth);
		~Direct3D();

	private:
		bool vsyncEnabled;
		DXGI_ADAPTER_DESC adapterInfo;

		// DirectX internal types
		IDXGISwapChain* swapChain;
		ID3D11Device* device;
		ID3D11DeviceContext* deviceContext;
		ID3D11RenderTargetView* renderTargetView;
		ID3D11Texture2D* depthStencilBuffer;
		ID3D11DepthStencilState* depthStencilState;
		ID3D11DepthStencilView* depthStencilView;
		ID3D11RasterizerState* rasterState;

		// Transformation matrices to convert data to a perspective 2D
		// projection, orthographic (flat 2D) projection, or from 
		// model/view space to world space
		Matrix4 perspProjector;
		Matrix4 orthoProjector;
		Matrix4 worldMatrix;
};
