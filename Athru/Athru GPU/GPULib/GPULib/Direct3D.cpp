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

	// Check swap-chain description & instantiation here

	// Instantiate the swap chain, Direct3D device, and Direct3D device context
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
	result = D3D12CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0,
										   D3D11_CREATE_DEVICE_DEBUG, &featureLevel, 1,
										   D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &device,
										   nullptr, &deviceContext);
	assert(SUCCEEDED(result));

	// Set up the DirectX debug layer
	result = device->QueryInterface(__uuidof(debugDevice),
									(void**)&debugDevice); // Should ask why this works...
	assert(SUCCEEDED(result));

	// Cache the address of the back buffer
	Microsoft::WRL::ComPtr<ID3D12Texture2D> backBufferPtr;
	result = swapChain->GetBuffer(0,
								  __uuidof(ID3D12Texture2D),
								  (void**)&backBufferPtr);
	assert(SUCCEEDED(result));

	// Create the render target view with the back buffer and verify that it isn't stored
	// in [nullptr]
	assert(backBufferPtr != nullptr);
	result = device->CreateRenderTargetView(backBufferPtr.Get(), NULL, &defaultRenderTarget);
	assert(SUCCEEDED(result));

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
	// *much* more wrapping code needed here, compare with Microsoft's practice project
	swapChain->Present(GraphicsStuff::VSYNC_ENABLED, 0);

	// Flipped-sequential double-buffering unbinds the render-target, so rebind it here
	//graphicsCmdList->OMSetRenderTargets(1, defaultRenderTarget.GetAddressOf(), nullptr);
}

void Direct3D::InitRasterPipeline(const D3D12_SHADER_BYTECODE& vs,
								  const D3D12_SHADER_BYTECODE& ps,
								  const D3D12_INPUT_LAYOUT_DESC& inputLayout,
								  ID3D12RootSignature* rootSig,
								  const Microsoft::WRL::ComPtr<ID3D12PipelineState>& pipelineState,
								  const Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& graphicsCmdAlloc)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline;
	pipeline.pRootSignature = rootSig;
	pipeline.VS = vs;
	pipeline.PS = ps;
	pipeline.BlendState.AlphaToCoverageEnable = false;
	pipeline.BlendState.IndependentBlendEnable = false;
	pipeline.BlendState.RenderTarget[0].BlendEnable = false; // No blending/transparency in Athru
	pipeline.RasterizerState.DepthClipEnable = false; // Try to guarantee that the presentation quad will never clip out of the display
													  // (super unlikely but verifiable code is nice)
	pipeline.DepthStencilState.DepthEnable = false; // Strictly implicit rendering in Athru, no strong need for a depth buffer
	pipeline.InputLayout = inputLayout; // Pass along the provided input layout
	pipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; // No discontinuous index-buffers in Athru
	pipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // Only triangular geometry in Athru (representing the display quad)
	pipeline.NumRenderTargets = 1; // Only one render target atm
	pipeline.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	pipeline.SampleDesc.Count = 1;
	pipeline.SampleDesc.Quality = 0; // We're literally only using the raster pipeline to present path-tracing results to the display; heavy
									 // filtering within our own compute pipeline means we'll be ok with single-sample minimal-quality MSAA
									 // (basically we're using our own AA solution so we don't need to enable it on GPU as well)
	pipeline.NodeMask = 0; // No multi-gpu support for now (*might* use the igpu as a co-processor for data analytics/UI rendering in the future)
	pipeline.CachedPSO.CachedBlobSizeInBytes = 0;
	pipeline.CachedPSO.pCachedBlob = nullptr; // No cached PSO stuff *rn* but might use it in the future; could
											  // save some setup time on startup
	pipeline.Flags = D3D12_PIPELINE_STATE_FLAG_NONE; // No flags atm
	// Construct, assign pipeline state here
	device->CreateGraphicsPipelineState(&pipeline,
										__uuidof(ID3D12PipelineState),
										(void**)pipelineState.GetAddressOf());
	HRESULT hr = graphicsCmdList->Reset(graphicsCmdAlloc.Get(), pipelineState.Get());
	assert(SUCCEEDED(hr));
	graphicsCmdList->SetPipelineState(pipelineState.Get());
	graphicsCmdList->Close(); // Might be worth extending this and [ExecuteCommandLists] later to pack more work into the first graphics submission
	ID3D12CommandList* listPttr[1] = { graphicsCmdList.Get() };
	graphicsQueue->ExecuteCommandLists(1, listPttr);
}

const Microsoft::WRL::ComPtr<ID3D12Device>& Direct3D::GetDevice()
{
	return device;
}

const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& Direct3D::GetGraphicsQueue()
{
	return graphicsQueue;
}

const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& Direct3D::GetGraphicsCmdList()
{
	return graphicsCmdList;
}

const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& Direct3D::GetComputeQueue()
{
	return computeQueue;
}

const Microsoft::WRL::ComPtr<ID3D12CommandList>& Direct3D::GetComputeCmdList()
{
	return computeCmdList;
}

const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& Direct3D::GetCopyQueue()
{
	return copyQueue;
}

const Microsoft::WRL::ComPtr<ID3D12CommandList>& Direct3D::GetCopyCmdList()
{
	return copyCmdList;
}

const DXGI_ADAPTER_DESC3& Direct3D::GetAdapterInfo()
{
	return adapterInfo;
}
