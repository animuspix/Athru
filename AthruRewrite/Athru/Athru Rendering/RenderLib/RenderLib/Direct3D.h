#pragma once

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3dcommon.h>
#include <d3d11.h>

class Direct3D
{
	public:
		Direct3D(HWND hwnd);
		~Direct3D();

		// Begin a draw session (flush the render texture and clear the depth buffer)
		void BeginScene();

		// End a draw session (publish to the screen and close the draw buffer)
		void EndScene();

		// Retrieve the device/device context
		ID3D11Device* GetDevice();
		ID3D11DeviceContext* GetDeviceContext();

		// Retrieve a reference to the render viewport used with [this]
		D3D11_VIEWPORT& GetViewport();

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
};

