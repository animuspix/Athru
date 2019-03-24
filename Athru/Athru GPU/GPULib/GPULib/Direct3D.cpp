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

	// Create a DXGI builer object
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
					AthruCore::Utility::AccessLogger()->Log("DX12 support on discrete GPU available, creating device");
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
}

Direct3D::~Direct3D() {}

void Direct3D::Present(const Microsoft::WRL::ComPtr<ID3D12Resource>& displayTex,
					 const D3D12_RESOURCE_STATES& dispTexState)
{
	// Copy the the display texture into the current back-buffer
	// Direct copies don't seem to work (can only access the zeroth back-buffer index?), should ask
	// CGSE about options here
	D3D12_TEXTURE_COPY_LOCATION texSrc;
	texSrc.pResource = displayTex.Get();
	texSrc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	texSrc.SubresourceIndex = 0; // No mip-maps on the display texture
	D3D12_TEXTURE_COPY_LOCATION texDst;
	Microsoft::WRL::ComPtr<ID3D12Resource> backBufTx; // Not sure about initialisation here
	HRESULT res = swapChain->GetBuffer(TimeStuff::frameCtr % 3, __uuidof(ID3D12Resource*), (void**)backBufTx.GetAddressOf());
	assert(SUCCEEDED(false));
	texDst.pResource = backBufTx.Get();
	texDst.SubresourceIndex = 0; // No mip-maps on the back-buffer
	texDst.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

	// Prepare resource barriers
	D3D12_RESOURCE_BARRIER copyBarriers[2];
	copyBarriers[0] = AthruGPU::TransitionBarrier(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST,
												  backBufTx,
												  D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT);
	copyBarriers[1] = AthruGPU::TransitionBarrier(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE,
												  displayTex,
												  dispTexState);

	// Transition to copyable resources & perform copy to the back-buffer
	graphicsCmdList->ResourceBarrier(2, copyBarriers);
	graphicsCmdList->CopyTextureRegion(&texDst, 0, 0, 0, &texSrc, nullptr); // Perform copy to the back-buffer

	// Transition back to standard-use resources
	copyBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	copyBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT;
	copyBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE;
	copyBarriers[1].Transition.StateAfter = dispTexState;
	graphicsCmdList->ResourceBarrier(2, copyBarriers);
	graphicsCmdList->Close(); // End graphics submissions for the current frame
    graphicsQueue->ExecuteCommandLists(1, (ID3D12CommandList**)graphicsCmdList.GetAddressOf()); // Execute graphics commands
    // -- Might need an extra sync here, unsure --

	// Pass the back-buffer up to the output monitor/Win64 surface
	// If vsync is enabled (read: equal to [true] or [1]), present the data
	// in sync with the screen refresh rate
	//
	// If vsync is disabled (read: equal to [false] or [0]), present it as
	// fast as possible
	HRESULT hr = swapChain->Present(GraphicsStuff::VSYNC_ENABLED, 0);
	assert(SUCCEEDED(hr)); // If present fails, something is wrong

	// Schedule the next frame...? Ask CGSE about this
}

void Direct3D::InitRasterPipeline(const D3D12_SHADER_BYTECODE& vs,
								  const D3D12_SHADER_BYTECODE& ps,
								  const D3D12_INPUT_LAYOUT_DESC& inputLayout,
								  ID3D12RootSignature* rootSig,
								  const Microsoft::WRL::ComPtr<ID3D12PipelineState>& pipelineState)
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
	HRESULT hr = device->CreateGraphicsPipelineState(&pipeline,
													 __uuidof(ID3D12PipelineState),
													 (void**)pipelineState.GetAddressOf());
	assert(SUCCEEDED(hr));
	activeGraphicsState = pipelineState; // Update active graphics pipeline state
	graphicsCmdList->SetPipelineState(pipelineState.Get()); // Assumed called within a broader command-list reset/execution block
}

void Direct3D::UpdateComputePipeline(const Microsoft::WRL::ComPtr<ID3D12PipelineState>& pipelineState)
{
	activeComputeState = pipelineState; // Update active compute pipeline state
	computeCmdList->SetPipelineState(pipelineState.Get()); // Assign new pipeline state within the API
														   // Assumed called within a broader command-list reset/execution block
}

AthruGPU::GPUMemory& Direct3D::GetGPUMem()
{
	return *gpuMem;
}

const Microsoft::WRL::ComPtr<ID3D12PipelineState>& Direct3D::GetGraphicsState()
{
	return activeGraphicsState;
}

const Microsoft::WRL::ComPtr<ID3D12PipelineState>& Direct3D::GetComputeState()
{
	return activeComputeState;
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
