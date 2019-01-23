#include <assert.h>
#include "Typedefs.h"
#include "UtilityServiceCentre.h"
#include "Direct3D.h"

Direct3D::Direct3D(HWND hwnd)
{
	// Value used to store success/failure for different DirectX operations
	u4Byte result;

	// Struct containing a description of the swap chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc;

	// Set up the swap chain description

	// Zero the memory in the swap chain description
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	// Initialise the swap chain sample description
	swapChainDesc.SampleDesc = DXGI_SAMPLE_DESC();

	// Request triple-buffering
	swapChainDesc.BufferCount = 3;

	// Set the width and height of each buffer to the display
	// dimensions
	swapChainDesc.BufferDesc.Width = GraphicsStuff::DISPLAY_WIDTH;
	swapChainDesc.BufferDesc.Height = GraphicsStuff::DISPLAY_HEIGHT;

	// Expect 8bpc RGBA color for the image buffers
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Define buffer usage
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Connect the swap-chain to the application window
	swapChainDesc.OutputWindow = hwnd;

	// Disable multisampling
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// Set fullscreen/windowed mode
	swapChainDesc.Windowed = !GraphicsStuff::FULL_SCREEN;

	// Allow the rasterizer to decide the most appropriate scan-line ordering and scaling
	// mode
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Discard buffered contents after presenting; the [flip] option might be slightly
	// faster than otherwise
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	// Don't set any extra swap chain settings
	swapChainDesc.Flags = 0;

	if (GraphicsStuff::VSYNC_ENABLED)
	{
		// Figure out the monitor refresh rate so we can implement vsync
		DEVMODE displayData = DEVMODE();
		EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &displayData);

		// Set the back buffer to refresh at the same rate as the monitor (implement vsync)
		swapChainDesc.BufferDesc.RefreshRate.Numerator = displayData.dmDisplayFrequency;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	else
	{
		// Set up swap chain refresh rate and presentation frequency without vsync
		// Set the back buffer to refresh as fast as possible
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// Instantiate the swap chain, Direct3D device, and Direct3D device context
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
	result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0,
										   D3D11_CREATE_DEVICE_DEBUG, &featureLevel, 1,
										   D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &device,
										   nullptr, &deviceContext);
	assert(SUCCEEDED(result));

	// Set up the DirectX debug layer
	result = device->QueryInterface(__uuidof(debugDevice),
									(void**)&debugDevice); // Should ask why this works...
	assert(SUCCEEDED(result));

	// Cache the address of the back buffer
	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBufferPtr;
	result = swapChain->GetBuffer(0,
								  __uuidof(ID3D11Texture2D),
								  (void**)&backBufferPtr);
	assert(SUCCEEDED(result));

	// Create the render target view with the back buffer and verify that it isn't stored
	// in [nullptr]
	assert(backBufferPtr != nullptr);
	result = device->CreateRenderTargetView(backBufferPtr.Get(), NULL, &defaultRenderTarget);
	assert(SUCCEEDED(result));

	// Send the render target view into the output pipeline; Athru uses strictly implicit rendering, so
	// no need for defined depth-buffer
	deviceContext->OMSetRenderTargets(1, defaultRenderTarget.GetAddressOf(), nullptr);

	// Describe the rasterizer
	D3D11_RASTERIZER_DESC rasterDesc;
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = false;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	// Instantiate the rasterizer state from the raster description
	// and store it within [rasterState]
	result = device->CreateRasterizerState(&rasterDesc, &rasterState);
	assert(SUCCEEDED(result));

	// Match the internal rasterizer state to the rasterizer state instantiated
	// above
	deviceContext->RSSetState(rasterState.Get());

	// Create the Direct3D viewport
	D3D11_VIEWPORT viewport;
	viewport.Width = (float)GraphicsStuff::DISPLAY_WIDTH;
	viewport.Height = (float)GraphicsStuff::DISPLAY_HEIGHT;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	deviceContext->RSSetViewports(1, &viewport);

	// Raise an error if any DirectX components failed to build
	assert(SUCCEEDED(result));
}

Direct3D::~Direct3D() {}

void Direct3D::Output()
{
	// Submit buffered dispatch/draw data to the GPU
	// If vsync is enabled (read: equal to [true] or [1]), present the data
	// in sync with the screen refresh rate
	//
	// If vsync is disabled (read: equal to [false] or [0]), present it as
	// fast as possible
	swapChain->Present(GraphicsStuff::VSYNC_ENABLED, 0);

	// Flipped-sequential double-buffering unbinds the render-target, so rebind it here
	deviceContext->OMSetRenderTargets(1, defaultRenderTarget.GetAddressOf(), nullptr);
}

const Microsoft::WRL::ComPtr<ID3D11Device>& Direct3D::GetDevice()
{
	return device;
}

const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& Direct3D::GetDeviceContext()
{
	return deviceContext;
}

const DXGI_ADAPTER_DESC& Direct3D::GetAdapterInfo()
{
	return adapterInfo;
}
