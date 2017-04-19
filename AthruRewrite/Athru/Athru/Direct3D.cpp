#include <assert.h>
#include "MathIncludes.h"
#include "typedefs.h"
#include "Graphics.h"
#include "Direct3D.h"

Direct3D::Direct3D(HWND hwnd)
{
	// Long integer used to store success/failure for different DirectX operations
	eightByteUnsigned result;

	// Struct containing a description of the swap chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc;

	// Set up the swap chain description

	// Zero the memory in the swap chain description
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	// Initialise the swap chain sample description
	swapChainDesc.SampleDesc = DXGI_SAMPLE_DESC();

	// Set a single back buffer
	swapChainDesc.BufferCount = 1;

	// Set the width and height of the back buffer
	swapChainDesc.BufferDesc.Width = DISPLAY_WIDTH;
	swapChainDesc.BufferDesc.Height = DISPLAY_HEIGHT;

	// Set regular 32-bit surface for the back buffer
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Set the usage of the back buffer.
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Set the handle for the window to render to.
	swapChainDesc.OutputWindow = hwnd;

	// Turn multisampling off.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// Set fullscreen/windowed mode
	swapChainDesc.Windowed = !FULL_SCREEN;

	// Allow the rasterizer to decide the most appropriate scan-line ordering and scaling
	// mode
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Discard the back buffer contents after presenting
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// Don't set any extra swap chain settings
	swapChainDesc.Flags = 0;

	if (VSYNC_ENABLED)
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

	// Set the expected feature level to DirectX 11
	D3D_FEATURE_LEVEL featureLevel;
	featureLevel = D3D_FEATURE_LEVEL_11_0;

	// Instantiate the swap chain, Direct3D device, and Direct3D device context
	deviceContext = nullptr;
	swapChain = nullptr;
	result = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, &featureLevel, 1,
										   D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &device, NULL, &deviceContext);

	assert(SUCCEEDED(result));

	// Cache the address of the back buffer
	ID3D11Texture2D* backBufferPtr;
	result = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferPtr);

	// Create the render target view with the back buffer and verify that it isn't stored
	// in [nullptr] (the program isn't allowed to de-reference null pointers, so the best
	// way to stop that from happening is to stamp out )
	assert(backBufferPtr != nullptr);
	result = device->CreateRenderTargetView(backBufferPtr, NULL, &defaultRenderTarget);

	// No more need for a pointer to the back buffer, so release it
	backBufferPtr->Release();
	backBufferPtr = nullptr;

	// Struct containing a description of the depth-stencil buffer
	D3D11_TEXTURE2D_DESC depthStencilBufferDesc;

	// Zero the memory in the depth buffer description
	ZeroMemory(&depthStencilBufferDesc, sizeof(depthStencilBufferDesc));

	// Set up the depth-stencil buffer description
	depthStencilBufferDesc.Width = DISPLAY_WIDTH;
	depthStencilBufferDesc.Height = DISPLAY_HEIGHT;
	depthStencilBufferDesc.MipLevels = 1;
	depthStencilBufferDesc.ArraySize = 1;
	depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilBufferDesc.SampleDesc.Count = 1;
	depthStencilBufferDesc.SampleDesc.Quality = 0;
	depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilBufferDesc.CPUAccessFlags = 0;
	depthStencilBufferDesc.MiscFlags = 0;

	// Generate the depth buffer texture using the depth-stencil buffer description
	result = device->CreateTexture2D(&depthStencilBufferDesc, NULL, &depthStencilBuffer);
	assert(SUCCEEDED(result));

	// Struct containing a description of the depth stencil
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;

	// Zero the memory in the depth stencil description
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

	// Set up the stencil state description
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;

	// Describe stencil operations for front-facing pixels
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Describe stencil operations for back-facing pixels
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Instantiate the depth stencil state within [depthStencilState]
	result = device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState);
	assert(SUCCEEDED(result));

	// Match the internal depth stencil state to the depth stencil state
	// instantiated above
	deviceContext->OMSetDepthStencilState(depthStencilState, 1);

	// Struct containing a description of the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;

	// Zero the memory in the depth stencil view
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));

	// Set up the depth stencil view description.
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	// Instantiate the depth stencil view and verify that [depthStencilBuffer]
	// is not at [nullptr] (since de-referencing null-pointers usually breaks everything)
	assert(depthStencilBuffer != nullptr);
	result = device->CreateDepthStencilView(depthStencilBuffer, &depthStencilViewDesc, &depthStencilView);
	assert(SUCCEEDED(result));

	// Send the render target view and depth stencil buffer into the output render pipeline
	deviceContext->OMSetRenderTargets(1, &defaultRenderTarget, depthStencilView);

	// Struct containing a description of the rasterizer state
	D3D11_RASTERIZER_DESC rasterDesc;

	// Setup the raster description
	rasterDesc.AntialiasedLineEnable = true;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
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
	deviceContext->RSSetState(rasterState);

	// Setup alpha blending description
	D3D11_BLEND_DESC BlendState;
	ZeroMemory(&BlendState, sizeof(D3D11_BLEND_DESC));
	BlendState.RenderTarget[0].BlendEnable = TRUE;
	BlendState.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	BlendState.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	BlendState.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	BlendState.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	BlendState.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	BlendState.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	BlendState.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	// Instantiate the blend state
	ID3D11BlendState* g_pBlendStateBlending = NULL;
	device->CreateBlendState(&BlendState, &g_pBlendStateBlending);

	// Match the internal blend state to the blend state instantiated above
	deviceContext->OMSetBlendState(g_pBlendStateBlending, NULL, 0xffffffff);

	// Struct representing a DirectX11 viewport
	D3D11_VIEWPORT viewport;

	// Setup the viewport for rendering
	viewport.Width = (float)DISPLAY_WIDTH;
	viewport.Height = (float)DISPLAY_HEIGHT;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	// Instantiate the viewport within [viewport]
	deviceContext->RSSetViewports(1, &viewport);

	// Create the perspective projection matrix
	perspProjector = DirectX::XMMatrixPerspectiveFovLH(VERT_FIELD_OF_VIEW_RADS, DISPLAY_ASPECT_RATIO, SCREEN_NEAR, SCREEN_FAR);

	// Create world matrix (initialized to the 4D identity matrix)
	worldMatrix = DirectX::XMMatrixIdentity();

	// Create orthographic projection matrix
	orthoProjector = DirectX::XMMatrixOrthographicLH(FRUSTUM_WIDTH_AT_NEAR, FRUSTUM_HEIGHT_AT_NEAR, SCREEN_NEAR, SCREEN_FAR);

	// Raise an error if any DirectX components failed to build
	assert(SUCCEEDED(result));
}

Direct3D::~Direct3D()
{
	// Set the application to windowed mode; otherwise, releasing the swap chain throws an exception
	//swapChain->SetFullscreenState(false, NULL);

	rasterState->Release();
	rasterState = nullptr;

	depthStencilView->Release();
	depthStencilView = nullptr;

	depthStencilState->Release();
	depthStencilState = nullptr;

	depthStencilBuffer->Release();
	depthStencilBuffer = nullptr;

	defaultRenderTarget->Release();
	defaultRenderTarget = nullptr;

	deviceContext->Release();
	deviceContext = nullptr;

	device->Release();
	device = nullptr;

	swapChain->Release();
	swapChain = nullptr;
}

void Direct3D::BeginScene(ID3D11RenderTargetView* renderTargetTexture)
{
	// Pass the given render target onto the GPU
	deviceContext->OMSetRenderTargets(1, &renderTargetTexture, depthStencilView);

	// The color to display before anything is drawn to the scene
	float color[4] = { 0.0f, 0.6f, 0.6f, 1.0f };

	// Clear the back buffer and flush the view with [color]
	deviceContext->ClearRenderTargetView(renderTargetTexture, color);

	// Clear the depth buffer
	deviceContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void Direct3D::BeginPost()
{
	// Pass the default render target back onto the GPU
	deviceContext->OMSetRenderTargets(1, &defaultRenderTarget, depthStencilView);

	// The color to display before anything is drawn to the scene
	float color[4] = { 0.6f, 0.0f, 0.6f, 1.0f };

	// Clear the back buffer and flush the view with [color]
	deviceContext->ClearRenderTargetView(defaultRenderTarget, color);

	// Clear the depth buffer
	deviceContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void Direct3D::EndScene()
{
	// Finish the rendering pass by displaying the
	// buffered draw data
	// If vsync is enabled (read: equal to [true] or [1]), present the data
	// in sync with the screen refresh rate
	//
	// If vsync is disabled (read: equal to [false] or [0]), present it as
	// fast as possible
	swapChain->Present(VSYNC_ENABLED, 0);
}

ID3D11Device* Direct3D::GetDevice()
{
	return device;
}

ID3D11DeviceContext* Direct3D::GetDeviceContext()
{
	return deviceContext;
}

D3D11_VIEWPORT& Direct3D::GetViewport()
{
	return viewport;
}

const DXGI_ADAPTER_DESC& Direct3D::GetAdapterInfo()
{
	return adapterInfo;
}

DirectX::XMMATRIX Direct3D::GetPerspProjector()
{
	return perspProjector;
}

DirectX::XMMATRIX Direct3D::GetOrthoProjector()
{
	return orthoProjector;
}

DirectX::XMMATRIX Direct3D::GetWorldMatrix()
{
	return worldMatrix;
}
