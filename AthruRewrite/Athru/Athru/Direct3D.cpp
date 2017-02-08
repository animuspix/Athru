#include <assert.h>
#include "Direct3D.h"

Direct3D::Direct3D() {}
Direct3D::Direct3D(unsigned int screenWidth, unsigned int screenHeight, bool vsyncActive, HWND hwnd, bool isFullscreen, 
				   float screenFullDepth, float screenNearDepth)
{
	// Long integer used to store success/failure for different DirectX operations
	HRESULT result;
	IDXGIAdapter* adapter;
	IDXGIOutput* adapterOutput;
	unsigned int numModes, i, numerator, denominator, stringLength;
	int error;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	D3D_FEATURE_LEVEL featureLevel;
	ID3D11Texture2D* backBufferPtr;
	D3D11_TEXTURE2D_DESC depthBufferDesc;
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	D3D11_RASTERIZER_DESC rasterDesc;
	D3D11_VIEWPORT viewport;
	float fieldOfView, screenAspect;

	vsyncEnabled = vsyncActive;

	if (vsyncEnabled)
	{
		// Create DirectX graphics interface factory
		IDXGIFactory* factory;
		CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);

		// Use the factory to create an adapter for the primary graphics interface (video card)
		result = factory->EnumAdapters(0, &adapter);

		// Enumerate the available monitors
		result = adapter->EnumOutputs(0, &adapterOutput);

		// Generate + store a description of the video card
		adapterInfo = DXGI_ADAPTER_DESC();

		// Create an array to hold all the possible display modes for this monitor/video card combination
		DXGI_MODE_DESC displayModes[4095];

		// Ask the adapter for a collection of possible display modes
		result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModes);

		// Traverse the set of display modes until one appears with the given screen width and height;
		// cache the numerator/denominator of the refresh rate for the selected mode
		for (i = 0; i < numModes; i += 1)
		{
			if (displayModes[i].Width == screenWidth && displayModes[i].Height == screenHeight)
			{
				numerator = displayModes[i].RefreshRate.Numerator;
				denominator = displayModes[i].RefreshRate.Denominator;
				break;
			}
		}

		swapChainDesc.BufferDesc.RefreshRate.Numerator = numerator;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = denominator;
	}
		

	// Zero the memory in the swap chain description
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	// Consider querying why we only initialize the back buffer

	// Set a single back buffer
	swapChainDesc.BufferCount = 1;

	// Set the width and height of the back buffer
	swapChainDesc.BufferDesc.Width = screenWidth;
	swapChainDesc.BufferDesc.Height = screenHeight;

	// Set regular 32-bit surface for the back buffer
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Set the refresh rate of the back buffer.
	if (vsyncActive)
	{
	}
	else
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// Set the usage of the back buffer.
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Set the handle for the window to render to.
	swapChainDesc.OutputWindow = hwnd;

	// Turn multisampling off.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// Set fullscreen/windowed mode
	swapChainDesc.Windowed = !isFullscreen;

	// Allow the rasterizer to decide the most appropriate scan-line ordering and scaling
	// mode
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Discard the back buffer contents after presenting
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// Don't set any extra swap-chain settings
	swapChainDesc.Flags = 0;

	// Set the expected feature level to DirectX 11
	featureLevel = D3D_FEATURE_LEVEL_11_0;

	// Raise an error if anything failed to execute
	assert(FAILED(result));
}

Direct3D::~Direct3D()
{

}
