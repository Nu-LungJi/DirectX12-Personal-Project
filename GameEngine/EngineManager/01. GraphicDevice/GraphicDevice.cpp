#include "GraphicDevice.h"

#include "EngineDebugger.h"
#include "EngineDiagnostics.h"
#include "EngineLogger.h"
#include "LogTypeLookUp.h"
ENGINE_USE

HRESULT DescriptorAllocator::Initialize(ID3D12Device* Device, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t Capacity) {
    if (!Device || Capacity == 0) return E_INVALIDARG;
    if (m_pHeap) return E_UNEXPECTED;
    if (Type != D3D12_DESCRIPTOR_HEAP_TYPE_RTV && 
        Type != D3D12_DESCRIPTOR_HEAP_TYPE_DSV &&
        Type != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) return E_INVALIDARG;

    const bool ShaderVisible = Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    
    D3D12_DESCRIPTOR_HEAP_DESC Desc{};
    Desc.Type = Type;
    Desc.NumDescriptors = Capacity;
    Desc.Flags = ShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    HRESULT Result = Device->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(m_pHeap.GetAddressOf()));
    if (FAILED(Result)) return Result;

    m_iDescriptorSize = Device->GetDescriptorHandleIncrementSize(Type);
    m_bShaderVisible = ShaderVisible;

    m_AllocatedList.assign(Capacity, 0);
    m_FreeIndexList.reserve(Capacity);

    for (uint32_t i = Capacity; i > 0; --i)
        m_FreeIndexList.push_back(i - 1);

    return S_OK;
}

HRESULT DescriptorAllocator::Allocate(DescriptorAllocation& Allocation) {
    if (!m_pHeap) return E_UNEXPECTED;

    if (Allocation.Index != UINT32_MAX) return E_INVALIDARG;
    if (m_FreeIndexList.empty()) return E_OUTOFMEMORY;

    const uint32_t Index = m_FreeIndexList.back();
    m_FreeIndexList.pop_back();

    Allocation.Index = Index;
    Allocation.CPU = m_pHeap->GetCPUDescriptorHandleForHeapStart();
    Allocation.CPU.ptr += static_cast<SIZE_T>(Index) * m_iDescriptorSize;

    Allocation.GPU = {};

    if (m_bShaderVisible) {
        Allocation.GPU = m_pHeap->GetGPUDescriptorHandleForHeapStart();
        Allocation.GPU.ptr += static_cast<uint64_t>(Index) * m_iDescriptorSize;
    }

    m_AllocatedList[Index] = 1;
    
    return S_OK;
}

HRESULT DescriptorAllocator::Free(DescriptorAllocation& Allocation) {
    const uint32_t index = Allocation.Index;
    if (!m_pHeap || index >= m_AllocatedList.size()) return E_INVALIDARG;

    const SIZE_T expectedCPU = m_pHeap->GetCPUDescriptorHandleForHeapStart().ptr + static_cast<SIZE_T>(index) * m_iDescriptorSize;

    if (Allocation.CPU.ptr != expectedCPU || !m_AllocatedList[index])
        return E_INVALIDARG;

    m_AllocatedList[index] = 0;
    m_FreeIndexList.push_back(index);

    Allocation = {};
    return S_OK;
}

void DescriptorAllocator::Shutdown() {
    m_FreeIndexList.clear();
    m_AllocatedList.clear();

    m_pHeap.Reset();

    m_iDescriptorSize = 0;
    m_bShaderVisible = false;
}

GraphicDevice::~GraphicDevice() {
    Discard_FrameForShutdown();
    if (m_pCommandQueue && m_pFence && m_FenceEvent) {
        const HRESULT Result = Wait_ForGPU();
        if (FAILED(Result)) OutputDebugStringW(L"GraphicDevice: GPU wait failed during shutdown.\n");
    }

    Reset_Initialization();
}

HRESULT GraphicDevice::Initialize_DescriptorHeaps() {
    HRESULT Result = m_RTVAllocator.Initialize(m_pDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_GraphicDeviceDesc.m_iRTVDescriptorCount);
    if (FAILED(Result)) return Result;

    Result = m_DSVAllocator.Initialize(m_pDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, m_GraphicDeviceDesc.m_iDSVDescriptorCount);
    if (FAILED(Result)) return Result;

    return  m_ResourceAllocator.Initialize(m_pDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_GraphicDeviceDesc.m_iResourceDescriptorCount);
}

HRESULT GraphicDevice::Initialize(const GraphicDeviceDesc& Desc) {
    if (m_pDevice) return E_UNEXPECTED;

    if (!IsWindow(Desc.m_hWnd) ||  // Check WindowHandle is Valid and Registered on current system properly.
        Desc.m_iWindowWidth == 0 || Desc.m_iWindowHeight == 0 || // Window Scale must be over 1x1
        Desc.m_iBackBufferCount < 2  || Desc.m_iBackBufferCount > 16 ||     // BackBuffer should be over 3
        Desc.m_iRTVDescriptorCount < Desc.m_iBackBufferCount ||
        Desc.m_iDSVDescriptorCount == 0 ||
        Desc.m_iResourceDescriptorCount == 0)
        return E_INVALIDARG;

    m_GraphicDeviceDesc = Desc;

#if defined(_DEBUG) || defined(_PROFILE)
    // Enable DirectX debugging features.

    // Configure DRED before device creation.
    const HRESULT DiagnosticResult = EngineDiagnostics::Configure_DRED();

    if (FAILED(DiagnosticResult)) {
        (void)EngineLogger::Warning(LogChannels::Diagnostics, "DRED configuration failed: 0x{:08X}", 
            static_cast<uint32_t>(DiagnosticResult));
    }
#endif
    
    HRESULT Result = Initialize_Factory(Desc);

    if (SUCCEEDED(Result))
        Result = Initialize_Device(Desc);

    if (SUCCEEDED(Result))
        Result = Initialize_DescriptorHeaps();

    if (SUCCEEDED(Result))
        Result = Initialize_CommandQueue();

    if (SUCCEEDED(Result))
        Result = Initialize_Fence();

    if (SUCCEEDED(Result))
        Result = Initialize_FrameResources();

    if (SUCCEEDED(Result))
        Result = Initialize_SwapChain();

    if (SUCCEEDED(Result))
        Result = Initialize_BackBuffers();

    if (FAILED(Result)) {
        Reset_Initialization();
        return Result;
    }

    m_bInitialized = true;
    return S_OK;
}

HRESULT GraphicDevice::Wait_ForGPU() {
    if (!m_pCommandQueue || !m_pFence || !m_FenceEvent || m_bFrameStarted)
        return E_UNEXPECTED;

    const uint64_t Value = m_iNextFenceValue++;

    HRESULT Result = m_pCommandQueue->Signal(m_pFence.Get(), Value);
    if (FAILED(Result)) return Result;

    return Wait_ForFence(Value);
}

HRESULT GraphicDevice::Resize(uint32_t Width, uint32_t Height) {
    if (!m_bInitialized || m_bFrameStarted)
        return E_UNEXPECTED;

    if (Width == 0 || Height == 0) {
        m_bMinimized = true;
        return S_FALSE;
    }

    if (Width > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION || Height > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION)
        return E_INVALIDARG;

    m_bMinimized = false;

    if (Width == m_GraphicDeviceDesc.m_iWindowWidth && Height == m_GraphicDeviceDesc.m_iWindowHeight)
        return S_OK;

    HRESULT Result = Wait_ForGPU();

    if (SUCCEEDED(Result))
        Result = Reset_CommandRecording();

    if (SUCCEEDED(Result))
        Result = Resize_BackBuffers(Width, Height);

    if (FAILED(Result))
        m_bInitialized = false;

    return Result;  
}

HRESULT GraphicDevice::Shutdown() {
    Discard_FrameForShutdown();

    if (m_pCommandQueue && m_pFence && m_FenceEvent) {
        const HRESULT Result = Wait_ForGPU();
        Dump_DebugMessages();
        if (FAILED(Result)) return Result;
    }

    Reset_Initialization();
    
    return S_OK;
}

GraphicDeviceView GraphicDevice::Get_CurrentView() const noexcept {
    if (!m_bInitialized) return {};

    GraphicDeviceView View;
    View.BackBuffer     = m_BackBufferList[m_iFrameIndex].Resource.Get();
    View.RTV            = m_BackBufferList[m_iFrameIndex].RTV.CPU;
    View.Viewport       = m_Viewport;
    View.ScissorRect    = m_ScissorRect;

    return View;
}
	
HRESULT GraphicDevice::Initialize_Factory(const GraphicDeviceDesc& Desc) {
    UINT FactoryFlags = 0;

#ifdef _DEBUG
    if (Desc.m_bEnableDebugLayer || Desc.m_bEnableGpuValidation) {
        const HRESULT Result =
            EngineDebugger::Enable_DebugLayer(
                Desc.m_bEnableGpuValidation);

        if (FAILED(Result)) {
            (void)EngineLogger::Error(
                LogChannels::Diagnostics,
                "Debug Layer initialization failed: 0x{:08X}",
                static_cast<uint32_t>(Result));

            return Result;
        }

        FactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    return CreateDXGIFactory2(
        FactoryFlags,
        IID_PPV_ARGS(m_pFactory.GetAddressOf()));
}

HRESULT GraphicDevice::Initialize_Device(const GraphicDeviceDesc& Desc) {
    HRESULT Error = DXGI_ERROR_UNSUPPORTED;

    for (uint32_t idx = 0; ; ++idx) {
        ComPtr<IDXGIAdapter1> Adapter;

        HRESULT Result = m_pFactory->EnumAdapterByGpuPreference(idx, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(Adapter.GetAddressOf()));

        if (Result == DXGI_ERROR_NOT_FOUND) return Error;
        if (FAILED(Result)) return Result;

        DXGI_ADAPTER_DESC1 AdapterDesc{};

        Result = Adapter->GetDesc1(&AdapterDesc);
        if (FAILED(Result)) return Result;

        if (AdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        ComPtr<ID3D12Device> Device;

        Result = D3D12CreateDevice(Adapter.Get(), Desc.m_MinimumFeatureLevel, IID_PPV_ARGS(Device.GetAddressOf()));
        if (FAILED(Result)) {
            Error = Result;
            continue;
        }

        GraphicDeviceSupport Support{};

        Result = Query_DeviceResult(Device.Get(), Support);
        if (FAILED(Result)) {
            Error = Result;
            continue;
        }

        if (Support.MaximumShaderModel < Desc.m_MinimumShaderModel) {
            Error = DXGI_ERROR_UNSUPPORTED;
            continue;
        }
        
        // Final Device Selection
        m_pAdapter  = Adapter;
        m_pDevice   = Device;
        m_GraphicDeviceSupport   = Support;
        
        const HRESULT DiagnosticResult = m_Diagnostics.Initialize(m_pDevice.Get());

        if (FAILED(DiagnosticResult))
        {
            // Optional diagnostics failed.
            // Record a warning, but keep the selected device.
            return DiagnosticResult;
        }

        return S_OK;
    }
}

HRESULT GraphicDevice::Query_DeviceResult(ID3D12Device* Device, GraphicDeviceSupport& Support) const
{
    CD3DX12FeatureSupport FeatureSupport;

    HRESULT Result = FeatureSupport.Init(Device);
    if (FAILED(Result)) return Result;

    Support = {};

    Support.MaximumFeatureLevel       = FeatureSupport.MaxSupportedFeatureLevel();
    Support.MaximumShaderModel        = FeatureSupport.HighestShaderModel();
    Support.ResourceBindingTier       = FeatureSupport.ResourceBindingTier();
    
    Support.RaytracingTier            = FeatureSupport.RaytracingTier();
    Support.MeshShaderTier            = FeatureSupport.MeshShaderTier();
    Support.VariableShadingRateTier   = FeatureSupport.VariableShadingRateTier();

    Support.m_bSupportsWaveOps        = FeatureSupport.WaveOps() != FALSE;
    Support.m_bSupportsNative16Bit    = FeatureSupport.Native16BitShaderOpsSupported() != FALSE;

    BOOL SupportsTearing = FALSE;
    constexpr UINT boolSize = sizeof(SupportsTearing);

    Result = m_pFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &SupportsTearing, boolSize);

    Support.m_bSupportsTearing = SUCCEEDED(Result) && SupportsTearing != FALSE;

    return S_OK;
}

HRESULT GraphicDevice::Initialize_CommandQueue() {
    D3D12_COMMAND_QUEUE_DESC QueueDesc;
    QueueDesc.Type      = D3D12_COMMAND_LIST_TYPE_DIRECT;
    QueueDesc.Priority  = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    QueueDesc.Flags     = D3D12_COMMAND_QUEUE_FLAG_NONE;
    QueueDesc.NodeMask  = 0;
    
    return m_pDevice->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(m_pCommandQueue.GetAddressOf()));
}

HRESULT GraphicDevice::Initialize_Fence() {
    HRESULT Result = m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.GetAddressOf()));
    if (FAILED(Result)) return Result;

    m_FenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!m_FenceEvent) return HRESULT_FROM_WIN32(GetLastError());
    
    return S_OK;
}

HRESULT GraphicDevice::Initialize_FrameResources() {
    m_FrameList.resize(m_GraphicDeviceDesc.m_iBackBufferCount); // m_FrameList manage Backbuffer

    for (auto& Frame : m_FrameList) {
        HRESULT Result = m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(Frame.Allocator.GetAddressOf()));
        if (FAILED(Result)) return Result;
    }

    HRESULT Result = m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_FrameList[0].Allocator.Get(),
                                                  nullptr, IID_PPV_ARGS(m_pCommandList.GetAddressOf()));
    if (FAILED(Result)) return Result;
    
    return m_pCommandList->Close();
}

HRESULT GraphicDevice::Initialize_SwapChain() {
    m_bAllowTearing =  m_GraphicDeviceSupport.m_bSupportsTearing && m_GraphicDeviceDesc.m_bAllowTearing;
    
    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc;
    SwapChainDesc.Width              = m_GraphicDeviceDesc.m_iWindowWidth;
    SwapChainDesc.Height             = m_GraphicDeviceDesc.m_iWindowHeight;
    SwapChainDesc.Format             = m_GraphicDeviceDesc.m_BackBufferFormat;
    
    SwapChainDesc.SampleDesc.Count   = 1;
    SwapChainDesc.SampleDesc.Quality = 0;
    
    SwapChainDesc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    SwapChainDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.BufferCount        = m_GraphicDeviceDesc.m_iBackBufferCount;
    
    SwapChainDesc.Scaling            = DXGI_SCALING_STRETCH;
    SwapChainDesc.AlphaMode          = DXGI_ALPHA_MODE_IGNORE;
    SwapChainDesc.Flags              = m_bAllowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    SwapChainDesc.Stereo             = FALSE;

    ComPtr<IDXGISwapChain1> SwapChain;

    HRESULT Result = m_pFactory->CreateSwapChainForHwnd(m_pCommandQueue.Get(), m_GraphicDeviceDesc.m_hWnd,
       &SwapChainDesc, nullptr, nullptr, SwapChain.GetAddressOf());
    if (FAILED(Result)) return Result;

    // DXGI react Automatic Screen Transition(Window <-> FullScreen) by Command key(Alt + Enter) 
    // DXGI_MWA_NO_ALT_ENTER Flag deactivate Automatic Screen Transition. (Enforce implementation upon developers)
    Result = m_pFactory->MakeWindowAssociation(m_GraphicDeviceDesc.m_hWnd, DXGI_MWA_NO_ALT_ENTER);
    if (FAILED(Result)) return Result;

    Result = SwapChain.As(&m_pSwapChain);
    if (FAILED(Result)) return Result;

    m_iFrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
    
    return S_OK;
}

HRESULT GraphicDevice::Initialize_BackBuffers() {
    m_BackBufferList.resize(m_GraphicDeviceDesc.m_iBackBufferCount);

    for (uint32_t i = 0; i < m_GraphicDeviceDesc.m_iBackBufferCount; ++i) {
        auto& Buffer = m_BackBufferList[i];

        HRESULT Result = m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(Buffer.Resource.GetAddressOf()));
        if (FAILED(Result)) return Result;

        Result = m_RTVAllocator.Allocate(Buffer.RTV);
        if (FAILED(Result)) return Result;

        m_pDevice->CreateRenderTargetView(Buffer.Resource.Get(), nullptr, Buffer.RTV.CPU);
    }

    m_Viewport = {
        0.f, 0.f,                                           //  TopLeftX / TopLeftY
        static_cast<float>(m_GraphicDeviceDesc.m_iWindowWidth),     //  Window Scale (Width x Height)
        static_cast<float>(m_GraphicDeviceDesc.m_iWindowHeight),
        0.f, 1.f                                            //  Depth(Min, Max)
    };

    m_ScissorRect = {
        0, 0,
        static_cast<LONG>(m_GraphicDeviceDesc.m_iWindowWidth),
        static_cast<LONG>(m_GraphicDeviceDesc.m_iWindowHeight)
    };

    return S_OK;
}

HRESULT GraphicDevice::Wait_ForFence(uint64_t Value) const {
    if (!m_pFence || !m_FenceEvent) return E_UNEXPECTED;

    const uint64_t Completed = m_pFence->GetCompletedValue();

    HRESULT Result;
    if (Completed == UINT64_MAX) {
        Result =  m_pDevice->GetDeviceRemovedReason();
        return FAILED(Result) ? Result : DXGI_ERROR_DEVICE_REMOVED;
    }

    if (Completed >= Value) return S_OK;

    Result = m_pFence->SetEventOnCompletion(Value, m_FenceEvent);
    if (FAILED(Result)) return Result;

    const DWORD WaitResult = WaitForSingleObject(m_FenceEvent, INFINITE);

    if (WaitResult == WAIT_FAILED)   return HRESULT_FROM_WIN32(GetLastError());
    if (WaitResult != WAIT_OBJECT_0) return E_FAIL;

    return m_pFence->GetCompletedValue() == UINT64_MAX ? DXGI_ERROR_DEVICE_REMOVED : S_OK;
}

void GraphicDevice::Transition_BackBuffer(D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After) const {
    const auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_BackBufferList[m_iFrameIndex].Resource.Get(), Before, After);
    m_pCommandList->ResourceBarrier(1, &Barrier);
}

void GraphicDevice::Reset_Initialization() {
    m_bInitialized = false;
    m_bFrameStarted = false;
    m_bMinimized = false;
    
    m_pCommandList.Reset();
    m_FrameList.clear();
    m_BackBufferList.clear();
    m_pSwapChain.Reset();

    m_pCommandQueue.Reset();
    m_pFence.Reset();

    if (m_FenceEvent)
    {
        CloseHandle(m_FenceEvent);
        m_FenceEvent = nullptr;
    }

    m_ResourceAllocator.Shutdown();
    m_DSVAllocator.Shutdown();
    m_RTVAllocator.Shutdown();

    m_pDevice.Reset();
    m_pAdapter.Reset();
    m_pFactory.Reset();

    m_iNextFenceValue = 1;
    m_iFrameIndex = 0;
    m_bAllowTearing = false;

    m_Viewport = {};
    m_ScissorRect = {};
    m_GraphicDeviceDesc = {};
    m_GraphicDeviceSupport = {};
}

HRESULT GraphicDevice::Resize_BackBuffers(uint32_t Width, uint32_t Height) {
    Release_BackBuffers(); // When Resize Window scale, Release all Back-buffers and Reassign

    // Change Backbuffer Scale, Format with SwapChain Flag, BufferCount
    HRESULT Result = m_pSwapChain->ResizeBuffers(m_GraphicDeviceDesc.m_iBackBufferCount, Width, Height, 
        m_GraphicDeviceDesc.m_BackBufferFormat, m_bAllowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);

    if (FAILED(Result)) return Result;

    // if Resized successfully, Change GraphicDevice Setting(Scale, FrameIndex).
    m_GraphicDeviceDesc.m_iWindowWidth = Width;
    m_GraphicDeviceDesc.m_iWindowHeight = Height;
    m_iFrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

    // Wait_ForGPU에서 기존 프레임 작업 완료를 확인했음. 
    for (auto& Frame : m_FrameList)
        Frame.FenceValue = 0;

    return Initialize_BackBuffers();
}

HRESULT GraphicDevice::Reset_CommandRecording() const {
    auto& Frame = m_FrameList[m_iFrameIndex];

    HRESULT Result = Frame.Allocator->Reset();
    if (FAILED(Result)) return Result;

    Result = m_pCommandList->Reset(Frame.Allocator.Get(), nullptr);
    if (FAILED(Result)) return Result;

    return m_pCommandList->Close();
}

void GraphicDevice::Release_BackBuffers() {
    for (auto& Buffer : m_BackBufferList) {
        Buffer.Resource.Reset();
        if (Buffer.RTV.Index != UINT32_MAX) m_RTVAllocator.Free(Buffer.RTV);
    }

    m_BackBufferList.clear();
}

void GraphicDevice::Discard_FrameForShutdown() {
    if (!m_bFrameStarted) return;

    const HRESULT Result = m_pCommandList->Close();
    if (FAILED(Result)) OutputDebugStringW(L"Failed to close the discarded command list.\n");

    m_bFrameStarted = false;
    m_bInitialized = false;
}

HRESULT GraphicDevice::Begin_Frame()
{
    if (!m_bInitialized || m_bFrameStarted)
        return E_UNEXPECTED;
    
    if (m_bMinimized)
        return S_FALSE;
    
    m_iFrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

    auto& Frame = m_FrameList[m_iFrameIndex];

    HRESULT Result = Wait_ForFence(Frame.FenceValue);

    if (FAILED(Result))
        return Result;

    Result = Frame.Allocator->Reset();

    if (FAILED(Result))
        return Result;

    Result = m_pCommandList->Reset(Frame.Allocator.Get(), nullptr);

    if (FAILED(Result))
        return Result;

    ID3D12DescriptorHeap* Heaps[] = {
        m_ResourceAllocator.Get_Heap()
    };

    m_pCommandList->SetDescriptorHeaps(1, Heaps);

    Transition_BackBuffer(
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);

    m_bFrameStarted = true;
    return S_OK;
}

HRESULT GraphicDevice::End_Frame() {
    if (!m_bInitialized || !m_bFrameStarted) return E_UNEXPECTED;

    Transition_BackBuffer(D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    HRESULT Result = m_pCommandList->Close();
    m_bFrameStarted = false;

    if (FAILED(Result)) {
        m_bInitialized = false;
        return Result;
    }

    ID3D12CommandList* Lists[] = {m_pCommandList.Get()};
    m_pCommandQueue->ExecuteCommandLists(1, Lists);

    const UINT SyncInterval = m_GraphicDeviceDesc.m_bEnableVSync ? 1 : 0;
    const UINT Flags = !m_GraphicDeviceDesc.m_bEnableVSync && m_bAllowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0;

    const HRESULT PresentResult = m_pSwapChain->Present(SyncInterval, Flags);

    const uint64_t FenceValue = m_iNextFenceValue++;

    Result = m_pCommandQueue->Signal(m_pFence.Get(), FenceValue);

    if (FAILED(Result)) {
        m_bInitialized = false;
        return Result;
    }

    m_FrameList[m_iFrameIndex].FenceValue = FenceValue;

    if (FAILED(PresentResult)) {
        m_Diagnostics.Report_DeviceRemoval();
        m_bInitialized = false;
    }

    return PresentResult;
}