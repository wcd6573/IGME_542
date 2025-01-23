#include "Graphics.h"
#include <dxgi1_6.h>

// Tell the drivers to use high-performance GPU in multi-GPU systems (like laptops)
extern "C"
{
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001; // NVIDIA
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1; // AMD
}

namespace Graphics
{
	// Annonymous namespace to hold variables
	// only accessible in this file
	namespace
	{
		unsigned int currentBackBufferIndex = 0;
		bool apiInitialized = false;
		bool supportsTearing = false;
		bool vsyncDesired = false;
		BOOL isFullscreen = false;

		D3D_FEATURE_LEVEL featureLevel{};

	}
}

// Getters
bool Graphics::VsyncState() { return vsyncDesired || !supportsTearing || isFullscreen; }
std::wstring Graphics::APIName() 
{ 
	switch (featureLevel)
	{
	case D3D_FEATURE_LEVEL_10_0: return L"D3D10";
	case D3D_FEATURE_LEVEL_10_1: return L"D3D10.1";
	case D3D_FEATURE_LEVEL_11_0: return L"D3D11";
	case D3D_FEATURE_LEVEL_11_1: return L"D3D11.1";
	case D3D_FEATURE_LEVEL_12_0: return L"D3D12";
	case D3D_FEATURE_LEVEL_12_1: return L"D3D12.1";
	case D3D_FEATURE_LEVEL_12_2: return L"D3D12.2";
	default: return L"Unknown";
	}
}
unsigned int Graphics::SwapChainIndex() { return currentBackBufferIndex; }

// --------------------------------------------------------
// Initializes the Graphics API, which requires window details.
// 
// windowWidth     - Width of the window (and our viewport)
// windowHeight    - Height of the window (and our viewport)
// windowHandle    - OS-level handle of the window
// vsyncIfPossible - Sync to the monitor's refresh rate if available?
// --------------------------------------------------------
HRESULT Graphics::Initialize(unsigned int windowWidth, unsigned int windowHeight, HWND windowHandle, bool vsyncIfPossible)
{
	// Only initialize once
	if (apiInitialized)
		return E_FAIL;

	// Save desired vsync state, though it may be stuck "on" if
	// the device doesn't support screen tearing
	vsyncDesired = vsyncIfPossible;

	// Determine if screen tearing ("vsync off") is available
	// - This is necessary due to variable refresh rate displays
	Microsoft::WRL::ComPtr<IDXGIFactory5> factory;
	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
	{
		// Check for this specific feature (must use BOOL typedef here!)
		BOOL tearingSupported = false;
		HRESULT featureCheck = factory->CheckFeatureSupport(
			DXGI_FEATURE_PRESENT_ALLOW_TEARING,
			&tearingSupported,
			sizeof(tearingSupported));

		// Final determination of support
		supportsTearing = SUCCEEDED(featureCheck) && tearingSupported;
	}

	// This will hold options for DirectX initialization
	unsigned int deviceFlags = 0;

	// If we're in debug mode in visual studio, we also
	// want to enable the D3D12 debug layer to see some
	// errors and wwarnings in Visual Studio's output window
	// when things go wrong!
	#if defined(DEBUG) || defined(_DEBUG)
		ID3D12Debug* debugController;
		D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
		debugController->EnableDebugLayer();
	#endif

	// Create the D3D12 device and check which feature level
	// we can reliably use in our application
	{
		HRESULT createResult = D3D12CreateDevice(
			0,						// Not explicitly specifiying which adapter (GPU)
			D3D_FEATURE_LEVEL_11_0, // MIN level - NOT the level we'll necessarily turn on
			IID_PPV_ARGS(Device.GetAddressOf())); // Macros to grab necessary IDs of device
		if (FAILED(createResult))
			return createResult;

		// Now that we have a device, determine the maximum
		// feature level supported by the device
		D3D_FEATURE_LEVEL levelsToCheck[] = {
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_12_1,
		};
		D3D12_FEATURE_DATA_FEATURE_LEVELS levels = {};
		levels.pFeatureLevelsRequested = levelsToCheck;
		levels.NumFeatureLevels = ARRAYSIZE(levelsToCheck);
		Device->CheckFeatureSupport(
			D3D12_FEATURE_FEATURE_LEVELS,
			&levels,
			sizeof(D3D12_FEATURE_DATA_FEATURE_LEVELS));
		featureLevel = levels.MaxSupportedFeatureLevel;
	}

#if defined(DEBUG) || defined(_DEBUG)
	// Set up a callback for any debug messages
	Device->QueryInterface(IID_PPV_ARGS(&InfoQueue));
#endif

	// Set up D3D12 command allocator / queue / list,
	// which are necessary pieces of issuing standard API calls
	{
		// Set up allocator
		Device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(CommandAllocator.GetAddressOf()));

		// Command queue
		D3D12_COMMAND_QUEUE_DESC qDesc = {};
		qDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		qDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		Device->CreateCommandQueue(&qDesc, IID_PPV_ARGS(CommandQueue.GetAddressOf()));

		// Command list
		Device->CreateCommandList(
			0, // Which physical GPU will handle these tasks? 0 for single GPU setup
			D3D12_COMMAND_LIST_TYPE_DIRECT,	// Type of command list
			CommandAllocator.Get(),			// The allocator for this list
			0,								// Initial pipeline state - none for now
			IID_PPV_ARGS(CommandList.GetAddressOf()));
	}

	// Swap chain creation
	{
		// Create a description of how our swap chain should work
		DXGI_SWAP_CHAIN_DESC swapDesc = {};
		swapDesc.BufferCount = NumBackBuffers;
		swapDesc.BufferDesc.Width = windowWidth;
		swapDesc.BufferDesc.Height = windowHeight;
		swapDesc.BufferDesc.RefreshRate.Numerator = 60;
		swapDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapDesc.Flags = supportsTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
		swapDesc.OutputWindow = windowHandle;
		swapDesc.SampleDesc.Count = 1;
		swapDesc.SampleDesc.Quality = 0;
		swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapDesc.Windowed = true;

		// Create a DXGI factor, which is what we use to create a swap chain
		Microsoft::WRL::ComPtr<IDXGIFactory> dxgiFactory;
		CreateDXGIFactory(IID_PPV_ARGS(dxgiFactory.GetAddressOf()));
		HRESULT swapResult = dxgiFactory->CreateSwapChain(
			CommandQueue.Get(), &swapDesc, SwapChain.GetAddressOf());
		if (FAILED(swapResult))
			return swapResult;
	}

	// What is the increment size between RTV descriptors in a descriptor heap?
	// This differs per GPU so we need to get it at applications start up
	SIZE_T RTVDescriptorSize =
		(SIZE_T)Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Create back buffers
	{
		// First create a descriptor heap for RTVs
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = NumBackBuffers;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(RTVHeap.GetAddressOf()));

		// Now create the RTV handles for each buffer (buffers were created by the swap chain)
		for (unsigned int i = 0; i < NumBackBuffers; i++)
		{
			// Grab this buffer from the swap chain
			SwapChain->GetBuffer(i, IID_PPV_ARGS(BackBuffers[i].GetAddressOf()));

			// Make a handle for it
			RTVHandles[i] = RTVHeap->GetCPUDescriptorHandleForHeapStart();
			RTVHandles[i].ptr += RTVDescriptorSize * i;

			// Create the render target view
			Device->CreateRenderTargetView(BackBuffers[i].Get(), 0, RTVHandles[i]);
		}
	}

	// Create depth/stencil buffer
	{
		// Create a descriptor heap for DSV
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(DSVHeap.GetAddressOf()));

		// Describe the depth stencil buffer resource
		D3D12_RESOURCE_DESC depthBufferDesc = {};
		depthBufferDesc.Alignment = 0;
		depthBufferDesc.DepthOrArraySize = 1;
		depthBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthBufferDesc.Height = windowHeight;
		depthBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthBufferDesc.MipLevels = 1;
		depthBufferDesc.SampleDesc.Count = 1;
		depthBufferDesc.SampleDesc.Quality = 0;
		depthBufferDesc.Width = windowWidth;

		// Describe the clear value tha twill most often be used
		// for this buffer (which optimizes the clearing of the buffer)
		D3D12_CLEAR_VALUE clear = {};
		clear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		clear.DepthStencil.Depth = 1.0f;
		clear.DepthStencil.Stencil = 0;

		// Describe the memory heap that will house this resource
		D3D12_HEAP_PROPERTIES props = {};
		props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		props.CreationNodeMask = 1;
		props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		props.Type = D3D12_HEAP_TYPE_DEFAULT;
		props.VisibleNodeMask = 1;

		// Actually create the resource, and the heap in which it
		// will reside, and map the resource to that heap
		Device->CreateCommittedResource(
			&props,
			D3D12_HEAP_FLAG_NONE,
			&depthBufferDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clear,
			IID_PPV_ARGS(DepthBuffer.GetAddressOf()));

		// Get the handle to the Depth Stencil View that we'll
		// be using for the depth buffer. The DSV is stored in
		// our DSV-specific descriptor heap.
		DSVHandle = DSVHeap->GetCPUDescriptorHandleForHeapStart();

		// Actually make the DSV
		Device->CreateDepthStencilView(
			DepthBuffer.Get(),
			0,	// Default view (first mip)
			DSVHandle);
	}

	// Create the fence for basic synchronization
	{
		Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(WaitFence.GetAddressOf()));
		WaitFenceEvent = CreateEventEx(0, 0, 0, EVENT_ALL_ACCESS);
		WaitFenceCounter = 0;
	}

	// Wait for the GPU before we proceed
	WaitForGPU();
	apiInitialized = true;
	return S_OK;
}

// --------------------------------------------------------
// Called at the end of the program to clean up any
// graphics API specific memory. 
// 
// This exists for completeness since D3D objects generally
// use ComPtrs, which get cleaned up automatically.  Other
// APIs might need more explicit clean up.
// --------------------------------------------------------
void Graphics::ShutDown()
{
}


// --------------------------------------------------------
// When the window is resized, the underlying 
// buffers (textures) must also be resized to match.
//
// If we don't do this, the window size and our rendering
// resolution won't match up.  This can result in odd
// stretching/skewing.
// 
// width  - New width of the window (and our viewport)
// height - New height of the window (and our viewport)
// --------------------------------------------------------
void Graphics::ResizeBuffers(unsigned int width, unsigned int height)
{
	// Ensure graphics API is initialized
	if (!apiInitialized)
		return;

	BackBufferRTV.Reset();
	DepthBufferDSV.Reset();

	// Resize the swap chain buffers
	SwapChain->ResizeBuffers(
		2, 
		width, 
		height, 
		DXGI_FORMAT_R8G8B8A8_UNORM, 
		supportsTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);

	// Grab the references to the first buffer
	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBufferTexture;
	SwapChain->GetBuffer(
		0,
		__uuidof(ID3D11Texture2D),
		(void**)backBufferTexture.GetAddressOf());

	// Now that we have the texture, create a render target view
	// for the back buffer so we can render into it.
	Device->CreateRenderTargetView(
		backBufferTexture.Get(),
		0,
		BackBufferRTV.GetAddressOf());

	// Set up the description of the texture to use for the depth buffer
	D3D11_TEXTURE2D_DESC depthStencilDesc = {};
	depthStencilDesc.Width = width;
	depthStencilDesc.Height = height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;

	// Create the depth buffer and its view, then 
	// release our reference to the texture
	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthBufferTexture;
	Device->CreateTexture2D(&depthStencilDesc, 0, &depthBufferTexture);
	Device->CreateDepthStencilView(
		depthBufferTexture.Get(),
		0,
		DepthBufferDSV.GetAddressOf()); 

	// Bind the views to the pipeline, so rendering properly 
	// uses their underlying textures
	Context->OMSetRenderTargets(
		1,
		BackBufferRTV.GetAddressOf(), // This requires a pointer to a pointer (an array of pointers), so we get the address of the pointer
		DepthBufferDSV.Get());

	// Lastly, set up a viewport so we render into
	// to correct portion of the window
	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = (float)width;
	viewport.Height = (float)height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	Context->RSSetViewports(1, &viewport);

	// Are we in a fullscreen state?
	SwapChain->GetFullscreenState(&isFullscreen, 0);
}


// --------------------------------------------------------
// Prints graphics debug messages waiting in the queue
// --------------------------------------------------------
void Graphics::PrintDebugMessages()
{
	// Do we actually have an info queue (usually in debug mode)
	if (!InfoQueue)
		return;

	// Any messages?
	UINT64 messageCount = InfoQueue->GetNumStoredMessages();
	if (messageCount == 0)
		return;

	// Loop and print messages
	for (UINT64 i = 0; i < messageCount; i++)
	{
		// Get the size so we can reserve space
		size_t messageSize = 0;
		InfoQueue->GetMessage(i, 0, &messageSize);

		// Reserve space for this message
		D3D11_MESSAGE* message = (D3D11_MESSAGE*)malloc(messageSize);
		InfoQueue->GetMessage(i, message, &messageSize);
		
		// Print and clean up memory
		if (message)
		{
			// Color code based on severity
			switch (message->Severity)
			{
			case D3D11_MESSAGE_SEVERITY_CORRUPTION:
			case D3D11_MESSAGE_SEVERITY_ERROR:
				printf("\x1B[91m"); break; // RED

			case D3D11_MESSAGE_SEVERITY_WARNING:
				printf("\x1B[93m"); break; // YELLOW

			case D3D11_MESSAGE_SEVERITY_INFO:
			case D3D11_MESSAGE_SEVERITY_MESSAGE:
				printf("\x1B[96m"); break; // CYAN
			}

			printf("%s\n\n", message->pDescription);
			free(message);

			// Reset color
			printf("\x1B[0m");
		}
	}

	// Clear any messages we've printed
	InfoQueue->ClearStoredMessages();
}
