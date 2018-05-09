#pragma once

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3dcommon.h>
#include <d3d11.h>
#include <wrl/client.h>

class Direct3D
{
	public:
		Direct3D(HWND hwnd);
		~Direct3D();

		// Begin a draw session (flush the render texture and clear the depth buffer)
		void BeginScene();

		// End a draw session (publish to the screen and close the draw buffer)
		void EndScene();

		// Retrieve the device/device context without incrementing their reference
		// counts
		const Microsoft::WRL::ComPtr<ID3D11Device>& GetDevice();
		const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& GetDeviceContext();

		// Retrieve a reference to the render viewport used with [this]
		//D3D11_VIEWPORT& GetViewport();

		// Retrieve a constant reference to information about the video adapter
		const DXGI_ADAPTER_DESC& GetAdapterInfo();

	private:
		// Stores whether or not vsync is enabled
		bool vsyncEnabled;

		// Stores information about the video adapter
		DXGI_ADAPTER_DESC adapterInfo;

		// Direct3D internal types
		Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
		Microsoft::WRL::ComPtr<ID3D11Device> device;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> defaultRenderTarget;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterState;
		Microsoft::WRL::ComPtr<ID3D11Debug> debugDevice;
};

