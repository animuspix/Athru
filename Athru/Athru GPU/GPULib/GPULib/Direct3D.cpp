#include <assert.h>
#include "Typedefs.h"
#include "UtilityServiceCentre.h"
#include "AthruResrc.h"
#include "Direct3D.h"
#include <DXGIDebug.h>

Direct3D::Direct3D(HWND hwnd)
{
	// Initialize the debug layer when _DEBUG is defined
	#ifdef _DEBUG
		//if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer))))
		//{ debugLayer->EnableDebugLayer(); }
	#endif

	// Create a DXGI builder object
	Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiBuilder = nullptr;
	u4Byte hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiBuilder));
	assert(SUCCEEDED(hr));

	// Find an appropriate GPU (first available for PIX captures, discrete otherwise) +
	// create the device
	Microsoft::WRL::ComPtr<IDXGIAdapter1> gpuHW = nullptr;
	bool dgpuFound = false;
	for (UINT adapterNdx = 0;
		 dxgiBuilder->EnumAdapters1(adapterNdx, &gpuHW) != DXGI_ERROR_NOT_FOUND;
		 adapterNdx += 1) // Iterate over all available adapters
	{
		DXGI_ADAPTER_DESC1 tmpGPUInfo;
		hr = gpuHW->GetDesc1(&tmpGPUInfo); // Get descriptions for iterated adapters
		// Select the first adapter with >=2GB dedicated video memory as a proxy for the discrete adapter
		// Can maybe check subsystem ID/vendor ID here instead
		if (tmpGPUInfo.DedicatedVideoMemory >= INT_MAX)
		{
			if (!(tmpGPUInfo.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) // Skip past any defined software adapters
			{
				dgpuFound = true;
				hr = D3D12CreateDevice(gpuHW.Get(), D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), &device);
				if (SUCCEEDED(hr))
				{
					hr = gpuHW->GetDesc1(&adapterInfo);
					assert(SUCCEEDED(hr));
					AthruCore::Utility::AccessLogger()->Log("DX12 support on discrete GPU available, creating device");
					break;
				}
				else
				{
					AthruCore::Utility::AccessLogger()->Log("Your graphics card does not appear to support DirectX12 \
															(required by the Athru game engine); exiting now"); // Should maybe use a popup instead of a logged error here
					AthruCore::Utility::AccessInput()->SetCloseFlag(); // Quit to desktop
					assert(false);
					return;
				}
			}
		}
	}

	// Error out if a discrete GPU couldn't be found in the system
	if (!dgpuFound)
	{
		AthruCore::Utility::AccessLogger()->Log("Athru requires at least 2GB of dedicated video memory; no matching GPU could be found"); // Should maybe use a popup instead of a logged error here
		AthruCore::Utility::AccessInput()->SetCloseFlag(); // Quit to desktop
		assert(false);
		return;
	}

	// Construct the GPU memory manager (creates generic heap + descriptor heaps + command allocators)
	gpuMem = new AthruGPU::GPUMemory(device);

	// Create the graphics command queue
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc;
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQueueDesc.NodeMask = 0x1; // Only one adapter atm
	hr = device->CreateCommandQueue(&cmdQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&graphicsQueue);
	assert(SUCCEEDED(hr));

	// Create the compute command queue
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	// Otherwise same settings as the graphics command queue
	hr = device->CreateCommandQueue(&cmdQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&computeQueue);
	assert(SUCCEEDED(hr));

	// Describe + construct the swap-chain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	if (GraphicsStuff::FULL_SCREEN) // Eventually import this from a settings file
							        // Implicitly prefer fullscreen windows to total fullscreen (might make that adjustable later)
	{
		u4Byte x, y;
		AthruCore::Utility::AccessApp()->GetMonitorRes(&x, &y);
		swapChainDesc.Width = x;
		swapChainDesc.Height = y;
	}
	else
	{
		swapChainDesc.Width = GraphicsStuff::DISPLAY_WIDTH; // Eventually import this from a settings file (updateable in-game)
		swapChainDesc.Height = GraphicsStuff::DISPLAY_HEIGHT; // Eventually import this from a settings file (updateable in-game)
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
											 &swapChain);

	// Create cpu/gpu synchronization fences + events
	for (u4Byte i = 0; i < 3; i += 1)
	{
		device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(sync[i]), (void**)&sync[i]);
		syncValues[i] = 1; // Fences change between [1] and [0] after synchronization
		syncEvt[i] = CreateEvent(NULL, FALSE, FALSE, L"AthruWaitEvent");
	}
}

Direct3D::~Direct3D()
{
	// Explicitly clear GPU heap memory
	gpuMem->~GPUMemory();

	// Explicitly clear swap-chain memory
	swapChain.Reset();

	// Report DXGI leaks
	// The DXGI factory + the hardware adapter have to stay around for this to run, so it will always list those on shutdown;
	// valid memory leaks will also prompt the generic DXGI leak warning after program exit
	#define LOG_DXGI_LEAKS
	#ifdef LOG_DXGI_LEAKS
		Microsoft::WRL::ComPtr<IDXGIDebug1> dxgiDebug;
		if (SUCCEEDED(DXGIGetDebugInterface1(NULL, IID_PPV_ARGS(&dxgiDebug))))
		{
			// Setup GUID for full DXGI debugging
			_GUID apiid;
			apiid.Data1 = 0xe48ae283;
			apiid.Data2 = 0xda80;
			apiid.Data3 = 0x490b;
			apiid.Data4[0] = 0x87;
			apiid.Data4[1] = 0xe6;
			apiid.Data4[2] = 0x43;
			apiid.Data4[3] = 0xe9;
			apiid.Data4[4] = 0xa9;
			apiid.Data4[5] = 0xcf;
			apiid.Data4[6] = 0xda;
			apiid.Data4[7] = 0x8;
			dxgiDebug->ReportLiveObjects(apiid, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
		}
	#endif
}

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

void Direct3D::GetBackBuf(const Microsoft::WRL::ComPtr<ID3D12Resource>& backBufTx, const u4Byte ndx)
{
    HRESULT res = swapChain->GetBuffer(ndx, __uuidof(ID3D12Resource*), (void**)backBufTx.GetAddressOf());
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

const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& Direct3D::GetComputeQueue()
{
	return computeQueue;
}

const DXGI_ADAPTER_DESC1& Direct3D::GetAdapterInfo()
{
	return adapterInfo;
}
