#include <assert.h>
#include "Typedefs.h"
#include "UtilityServiceCentre.h"
#include "AthruResrc.h"
#include "Direct3D.h"

Direct3D::Direct3D(HWND hwnd)
{
	// Initialize the debug layer when _DEBUG is defined
	#ifdef _DEBUG
		Microsoft::WRL::ComPtr<ID3D12Debug> debugLayer;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer))))
		{ debugLayer->EnableDebugLayer(); }
	#endif

	// Create a DXGI builder object
	Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiBuilder;
	u4Byte hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiBuilder));
	assert(SUCCEEDED(hr));

	// Cache a reference to the graphics adapter we'll be rendering with
	// Multi-gpu stuff (e.g. using the igpu for infrequent image/array processing) would begin forking here
	Microsoft::WRL::ComPtr<IDXGIAdapter1> gpuHW = nullptr;
	bool dx12GPU = false;
	bool cachedIGPU = false;
	DXGI_ADAPTER_DESC1 gpuInfo;
	for (UINT adapterNdx = 0;
		 dxgiBuilder->EnumAdapters1(adapterNdx, gpuHW.GetAddressOf()) != DXGI_ERROR_NOT_FOUND;
		 adapterNdx += 1) // Iterate over all available adapters
	{
		hr = gpuHW->GetDesc1(&gpuInfo); // Get descriptions for iterated adapters
		assert(SUCCEEDED(hr));
		if (gpuInfo.DedicatedVideoMemory >= INT_MAX) // Select the first adapter with >=2GB dedicated video memory as a proxy for the discrete adapter
													 // Can maybe check subsystem ID/vendor ID here instead
		{
			if (!(gpuInfo.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) // Skip past any defined software adapters
			{
				hr = D3D12CreateDevice(gpuHW.Get(), D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr); // Check for DX12 support without committing to device creation
				if (SUCCEEDED(hr))
				{
					dx12GPU = true;
					AthruCore::Utility::AccessLogger()->Log("DX12 support on discrete GPU available, creating device");
					break;
				}
			}
		}
		else if (!cachedIGPU)
		{
			// Default to the integrated gpu if discrete hardware is unavailable
			if (!(gpuInfo.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) // Skip past any defined software adapters
			{
				hr = D3D12CreateDevice(gpuHW.Get(), D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr); // Check for DX12 support without committing to device creation
				if (SUCCEEDED(hr))
				{
					dx12GPU = true;
					AthruCore::Utility::AccessLogger()->Log("DX12 support on integrated GPU available, creating device");
					break;
				}
			}
			cachedIGPU = true; // Ignore every integrated GPU available after the first one found in the system
		}
	}

	// DX12 support required to run Athru efficiently, so drop out of setup if it's unavailable
	if (!dx12GPU)
	{
		AthruCore::Utility::AccessLogger()->Log("Your display adapter (graphics card/integrated GPU) does not appear to support DirectX12 \
												 (required by the Athru game engine); exiting now"); // Should maybe use a popup instead of a logged error here
		AthruCore::Utility::AccessInput()->SetCloseFlag(); // Quit to desktop
		return;
	}

	// DX12 is supported, so create the device here ^_^
	hr = D3D12CreateDevice(gpuHW.Get(), D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), &device);
	assert(SUCCEEDED(hr));

	// Construct the GPU memory manager (creates generic heap + descriptor heaps + command allocators)
	gpuMem = new AthruGPU::GPUMemory(device);

	// Create the graphics command queue + allocator + command list
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc;
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQueueDesc.NodeMask = 0x1; // Only one adapter atm
	hr = device->CreateCommandQueue(&cmdQueueDesc, __uuidof(ID3D12CommandQueue), (void**)graphicsQueue.GetAddressOf());
	assert(SUCCEEDED(hr));
	assert(SUCCEEDED(device->CreateCommandAllocator(cmdQueueDesc.Type, __uuidof(ID3D12CommandAllocator), (void**)graphicsCmdAlloc.GetAddressOf())));
	assert(SUCCEEDED(device->CreateCommandList(0x1, cmdQueueDesc.Type, graphicsCmdAlloc.Get(), nullptr, __uuidof(ID3D12GraphicsCommandList), (void**)graphicsCmdList.GetAddressOf())));

	// Create the compute command queue + allocator
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	// Otherwise same settings as the graphics command queue
	hr = device->CreateCommandQueue(&cmdQueueDesc, __uuidof(ID3D12CommandQueue), (void**)computeQueue.GetAddressOf());
	assert(SUCCEEDED(hr));
	assert(SUCCEEDED(device->CreateCommandAllocator(cmdQueueDesc.Type, __uuidof(ID3D12CommandAllocator), (void**)computeCmdAlloc.GetAddressOf())));
	assert(SUCCEEDED(device->CreateCommandList(0x1, cmdQueueDesc.Type, computeCmdAlloc.Get(), nullptr, __uuidof(ID3D12GraphicsCommandList), (void**)computeCmdList.GetAddressOf())));

	// Create the copy command queue + allocator
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY;
	// Otherwise same settings as the compute command queue
	hr = device->CreateCommandQueue(&cmdQueueDesc, __uuidof(ID3D12CommandQueue), (void**)copyQueue.GetAddressOf());
	assert(SUCCEEDED(hr));
	assert(SUCCEEDED(device->CreateCommandAllocator(cmdQueueDesc.Type, __uuidof(ID3D12CommandAllocator), (void**)copyCmdAlloc.GetAddressOf())));
	assert(SUCCEEDED(device->CreateCommandList(0x1, cmdQueueDesc.Type, copyCmdAlloc.Get(), nullptr, __uuidof(ID3D12GraphicsCommandList), (void**)copyCmdList.GetAddressOf())));

	// Describe + construct the swap-chain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
	swapChainDesc.Width = GraphicsStuff::DISPLAY_WIDTH; // Eventually import this from a settings file (updateable in-game)
	swapChainDesc.Height = GraphicsStuff::DISPLAY_HEIGHT; // Eventually import this from a settings file (updateable in-game)
	if (GraphicsStuff::FULL_SCREEN) // Eventually import this from a settings file
							        // Implicitly prefer fullscreen windows to total fullscreen (might make that adjustable later)
	{
		swapChainDesc.Width = GetSystemMetrics(SM_CXSCREEN);
		swapChainDesc.Height = GetSystemMetrics(SM_CYSCREEN);
	}
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 8-bit swapchain (might consider 10-bit support when/if I get a HDR monitor)
	swapChainDesc.SampleDesc.Count = 1; // Manual surface/volume multisampling
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // This is the main presentation swap-chain
	swapChainDesc.BufferCount = 3; // Triple-buffering for miminal latency
	swapChainDesc.Scaling = DXGI_SCALING_NONE; // No image stretchiness for lower image resolutions (should import this from a settings file, too)
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapChainDesc.Flags = (GraphicsStuff::VSYNC_ENABLED) ? 0 : DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING; // Eventually import this from a settings file
																								   // Disabling vsync might be needed for hardware-sync systems like
																								   // G-Sync or Freesync
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_12_0;
	hr = dxgiBuilder->CreateSwapChainForHwnd(graphicsQueue.Get(), // Not actually the device pointer; see: https://docs.microsoft.com/en-us/windows/desktop/api/dxgi1_2/nf-dxgi1_2-idxgifactory2-createswapchainforhwnd
											 hwnd,
											 &swapChainDesc,
											 nullptr, // Implicitly prefer fullscreen windows to total fullscreen
											 nullptr, // No output restrictions (back-buffer can be displayed on any Win64 surface/any monitor)
											 swapChain.GetAddressOf());
	assert(SUCCEEDED(hr));

	// Create cpu/gpu synchronization fences + events
	for (u4Byte i = 0; i < 3; i += 1)
	{
		device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(sync[i]), (void**)&sync[i]);
		syncValues[i] = 1; // Fences change between [1] and [0] after synchronization
		syncEvt[i] = CreateEvent(NULL, FALSE, FALSE, L"WaitEvent");
	}
}

Direct3D::~Direct3D() {}

void Direct3D::Present()
{
	// Pass the back-buffer up to the output monitor/Win64 surface
	// If vsync is enabled (read: equal to [true] or [1]), present the data
	// in sync with the screen refresh rate
	//
	// If vsync is disabled (read: equal to [false] or [0]), present it as
	// fast as possible
	HRESULT hr = swapChain->Present(GraphicsStuff::VSYNC_ENABLED, 0);
	assert(SUCCEEDED(hr)); // If present fails, something is wrong
}

AthruGPU::GPUMemory& Direct3D::GetGPUMem()
{
	return *gpuMem;
}

void Direct3D::GetBackBuf(const Microsoft::WRL::ComPtr<ID3D12Resource>& backBufTx)
{
    HRESULT res = swapChain->GetBuffer(TimeStuff::frameCtr % 3, __uuidof(ID3D12Resource*), (void**)backBufTx.GetAddressOf());
    assert(SUCCEEDED(res));
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

const Microsoft::WRL::ComPtr<ID3D12CommandAllocator> Direct3D::GetGraphicsCmdAllocator()
{
	return graphicsCmdAlloc;
}

const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& Direct3D::GetComputeQueue()
{
	return computeQueue;
}

const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& Direct3D::GetComputeCmdList()
{
	return computeCmdList;
}

const Microsoft::WRL::ComPtr<ID3D12CommandAllocator> Direct3D::GetComputeCmdAllocator()
{
	return computeCmdAlloc;
}

const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& Direct3D::GetCopyQueue()
{
	return copyQueue;
}

const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& Direct3D::GetCopyCmdList()
{
	return copyCmdList;
}

const Microsoft::WRL::ComPtr<ID3D12CommandAllocator> Direct3D::GetCopyCmdAllocator()
{
	return copyCmdAlloc;
}

const DXGI_ADAPTER_DESC3& Direct3D::GetAdapterInfo()
{
	return adapterInfo;
}
