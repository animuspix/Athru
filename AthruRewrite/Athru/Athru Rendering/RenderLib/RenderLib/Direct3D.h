#pragma once

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")

#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3dcommon.h>
#include <d3d11.h>
#include <directxmath.h>

class Direct3D
{
	public:
		Direct3D(HWND hwnd);
		~Direct3D();

		// Begin a draw session (flush the render texture and clear the depth buffer)
		void BeginScene(ID3D11RenderTargetView* renderTarget);

		// Begin post processing (flush the default render target and clear the depth buffer)
		void BeginPost();

		// End a draw session (publish to the screen and close the draw buffer)
		void EndScene();

		// Retrieve the device/device context
		ID3D11Device* GetDevice();
		ID3D11DeviceContext* GetDeviceContext();

		// Retrieve a reference to the render viewport used with [this]
		D3D11_VIEWPORT& GetViewport();

		// Retrieve the perspective/orthographic projection matrices
		DirectX::XMMATRIX GetPerspProjector();
		DirectX::XMMATRIX GetOrthoProjector();

		// Retrieve the world matrix
		DirectX::XMMATRIX GetWorldMatrix();

		// Retrieve a constant reference to information about the video adapter
		const DXGI_ADAPTER_DESC& GetAdapterInfo();

	private:
		// Stores whether or not vsync is enabled
		bool vsyncEnabled;

		// Stores information about the video adapter
		DXGI_ADAPTER_DESC adapterInfo;

		// Stores information about the render viewport used with [this]
		D3D11_VIEWPORT viewport;

		// DirectX internal types
		IDXGISwapChain* swapChain;
		ID3D11Device* device;
		ID3D11DeviceContext* deviceContext;
		ID3D11RenderTargetView* defaultRenderTarget;
		ID3D11Texture2D* depthStencilBuffer;
		ID3D11DepthStencilState* depthStencilState;
		ID3D11DepthStencilView* depthStencilView;
		ID3D11RasterizerState* rasterState;
		ID3D11Debug* debugDevice;

		// Transformation matrices to convert data to a perspective 2D
		// projection or orthographic (flat 2D) projection
		DirectX::XMMATRIX perspProjector;
		DirectX::XMMATRIX orthoProjector;

		// Matrix representing the basic coordinate system used within
		// Athru; for more information on coordinate systems, see:
		// https://en.wikipedia.org/wiki/Coordinate_system
		DirectX::XMMATRIX worldMatrix;
};

