#include "GraphicDevice.h"
ENGINE_USE

HRESULT DescriptorAllocator::Initialize(ID3D12Device* Device, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t Capacity) {
    if (!Device || Capacity == 0) return E_INVALIDARG;
    if (m_pHeap) return E_UNEXPECTED;
    if (Type != D3D12_DESCRIPTOR_HEAP_TYPE_RTV && Type != D3D12_DESCRIPTOR_HEAP_TYPE_DSV &&
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
    Allocation.CPU.ptr += SIZE_T(Index) * m_iDescriptorSize;

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

    const SIZE_T expectedCPU = m_pHeap->GetCPUDescriptorHandleForHeapStart().ptr + SIZE_T(index) * m_iDescriptorSize;

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
    if (m_pCommandQueue && m_pFence && m_FenceEvent) {
        const HRESULT Result = Wait_ForGPU();
        if (FAILED(Result)) OutputDebugStringW(L"GraphicDevice: GPU wait failed during shutdown.\n");
    }

    Reset_Initialization();
}

HRESULT GraphicDevice::Initialize_DescriptorHeaps() {
    HRESULT Result = m_RTVAllocator.Initialize(m_pDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_Desc.RTVDescriptorCount);
    if (FAILED(Result)) return Result;

    Result = m_DSVAllocator.Initialize(m_pDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, m_Desc.DSVDescriptorCount);
    if (FAILED(Result)) return Result;

    return m_ResourceAllocator.Initialize(m_pDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_Desc.ResourceDescriptorCount);
}

HRESULT GraphicDevice::Initialize(const GraphicDeviceDesc& Desc)
{
    if (m_pDevice)
        return E_UNEXPECTED;

    if (!IsWindow(Desc.WindowHandle) ||
        Desc.WindowWidth == 0 ||
        Desc.WindowHeight == 0 ||
        Desc.BackBufferCount < 2 ||
        Desc.BackBufferCount > 16 ||
        Desc.RTVDescriptorCount < Desc.BackBufferCount ||
        Desc.DSVDescriptorCount == 0 ||
        Desc.ResourceDescriptorCount == 0)
        return E_INVALIDARG;

    m_Desc = Desc;

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

HRESULT GraphicDevice::Wait_ForGPU()
{
    if (!m_pCommandQueue || !m_pFence || !m_FenceEvent)
        return E_UNEXPECTED;

    const uint64_t Value = m_iNextFenceValue++;

    HRESULT Result = m_pCommandQueue->Signal(m_pFence.Get(), Value);

    if (FAILED(Result))
        return Result;

    return Wait_ForFence(Value);
}

HRESULT GraphicDevice::Resize(uint32_t Width, uint32_t Height) {
    if (!m_bInitialized || m_bFrameStarted)
        return E_UNEXPECTED;

    if (Width == 0 || Height == 0)
    {
        m_bMinimized = true;
        return S_FALSE;
    }

    if (Width > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION ||
        Height > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION)
        return E_INVALIDARG;

    m_bMinimized = false;

    if (Width == m_Desc.WindowWidth &&
        Height == m_Desc.WindowHeight)
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
    if (m_bFrameStarted)
        return E_UNEXPECTED;

    if (m_pCommandQueue && m_pFence && m_FenceEvent)
    {
        HRESULT Result = Wait_ForGPU();

        if (FAILED(Result))
            return Result;
    }

    Reset_Initialization();
    return S_OK;
}

GraphicDeviceView GraphicDevice::Get_CurrentView() const noexcept {
    if (!m_bInitialized) return {};

    GraphicDeviceView View{};
    View.BackBuffer     = m_BackBufferList[m_iFrameIndex].Resource.Get();
    View.RTV            = m_BackBufferList[m_iFrameIndex].RTV.CPU;
    View.Viewport       = m_Viewport;
    View.ScissorRect    = m_ScissorRect;

    return View;
}
	
HRESULT GraphicDevice::Initialize_Factory(const GraphicDeviceDesc& Desc) {
    UINT FactoryFlags = 0;

#ifdef _DEBUG
    if (Desc.EnableDebugLayer || Desc.EnableGpuValidation) {
        ComPtr<ID3D12Debug> DebugContext;

        HRESULT Result = D3D12GetDebugInterface(IID_PPV_ARGS(DebugContext.GetAddressOf()));
        if (FAILED(Result)) return Result;
        
        DebugContext->EnableDebugLayer();
        FactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

        if (Desc.EnableGpuValidation)
        {
            ComPtr<ID3D12Debug1> DebugContext1;

            Result = DebugContext.As(&DebugContext1);
            if (FAILED(Result)) return Result;

            DebugContext1->SetEnableGPUBasedValidation(TRUE);
        }
    }
#endif

    return CreateDXGIFactory2(FactoryFlags, IID_PPV_ARGS(m_pFactory.GetAddressOf()));
}

HRESULT GraphicDevice::Initialize_Device(const GraphicDeviceDesc& Desc) {
    HRESULT Error = DXGI_ERROR_UNSUPPORTED;

    for (uint32_t idx = 0; ; ++idx) {
        ComPtr<IDXGIAdapter1> Adapter;

        HRESULT Result = m_pFactory->EnumAdapterByGpuPreference(idx, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,IID_PPV_ARGS(Adapter.GetAddressOf()));

        if (Result == DXGI_ERROR_NOT_FOUND) return Error;
        if (FAILED(Result)) return Result;

        DXGI_ADAPTER_DESC1 adapterDesc{};

        Result = Adapter->GetDesc1(&adapterDesc);
        if (FAILED(Result)) return Result;

        if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        ComPtr<ID3D12Device> Device;

        Result = D3D12CreateDevice(Adapter.Get(), Desc.MinimumFeatureLevel, IID_PPV_ARGS(Device.GetAddressOf()));

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

        if (Support.MaximumShaderModel < Desc.MinimumShaderModel)
        {
            Error = DXGI_ERROR_UNSUPPORTED;
            continue;
        }

        m_pAdapter  = Adapter;
        m_pDevice   = Device;
        m_Support   = Support;

        return S_OK;
    }
}

HRESULT GraphicDevice::Query_DeviceResult(ID3D12Device* Device, GraphicDeviceSupport& Support) {
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

    Support.SupportsWaveOps           = FeatureSupport.WaveOps() != FALSE;
    Support.SupportsNative16Bit       = FeatureSupport.Native16BitShaderOpsSupported() != FALSE;

    BOOL SupportsTearing = FALSE;
    constexpr UINT boolSize = sizeof(SupportsTearing);

    Result = m_pFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &SupportsTearing, boolSize);

    Support.SupportsTearing = SUCCEEDED(Result) && SupportsTearing != FALSE;

    return S_OK;
}

HRESULT GraphicDevice::Initialize_CommandQueue() {
    D3D12_COMMAND_QUEUE_DESC QueueDesc{};
    QueueDesc.Type      = D3D12_COMMAND_LIST_TYPE_DIRECT;
    QueueDesc.Priority  = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

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
    m_FrameList.resize(m_Desc.BackBufferCount);

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
    m_bAllowTearing = m_Desc.AllowTearing && m_Support.SupportsTearing;

    DXGI_SWAP_CHAIN_DESC1 Desc{};
    Desc.Width              = m_Desc.WindowWidth;
    Desc.Height             = m_Desc.WindowHeight;
    Desc.Format             = m_Desc.BackBufferFormat;
    Desc.SampleDesc.Count   = 1;
    Desc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    Desc.BufferCount        = m_Desc.BackBufferCount;
    Desc.Scaling            = DXGI_SCALING_STRETCH;
    Desc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    Desc.AlphaMode          = DXGI_ALPHA_MODE_IGNORE;
    Desc.Flags              = m_bAllowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    ComPtr<IDXGISwapChain1> SwapChain;

    HRESULT Result = m_pFactory->CreateSwapChainForHwnd(m_pCommandQueue.Get(), m_Desc.WindowHandle,
                                                        &Desc, nullptr, nullptr, SwapChain.GetAddressOf());
    if (FAILED(Result)) return Result;

    Result = m_pFactory->MakeWindowAssociation(m_Desc.WindowHandle, DXGI_MWA_NO_ALT_ENTER);
    if (FAILED(Result)) return Result;

    Result = SwapChain.As(&m_pSwapChain);
    if (FAILED(Result)) return Result;

    m_iFrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
    
    return S_OK;
}

HRESULT GraphicDevice::Initialize_BackBuffers() {
    m_BackBufferList.resize(m_Desc.BackBufferCount);

    for (uint32_t i = 0; i < m_Desc.BackBufferCount; ++i) {
        auto& Buffer = m_BackBufferList[i];

        HRESULT Result = m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(Buffer.Resource.GetAddressOf()));
        if (FAILED(Result)) return Result;

        Result = m_RTVAllocator.Allocate(Buffer.RTV);
        if (FAILED(Result)) return Result;

        m_pDevice->CreateRenderTargetView(Buffer.Resource.Get(), nullptr, Buffer.RTV.CPU);
    }

    m_Viewport = {
        0.f, 0.f,
        static_cast<float>(m_Desc.WindowWidth),
        static_cast<float>(m_Desc.WindowHeight),
        0.f, 1.f
    };

    m_ScissorRect = {
        0, 0,
        static_cast<LONG>(m_Desc.WindowWidth),
        static_cast<LONG>(m_Desc.WindowHeight)
    };

    return S_OK;
}

HRESULT GraphicDevice::Wait_ForFence(uint64_t Value)
{
    if (!m_pFence || !m_FenceEvent)
        return E_UNEXPECTED;

    const uint64_t Completed = m_pFence->GetCompletedValue();

    if (Completed == UINT64_MAX)
        return DXGI_ERROR_DEVICE_REMOVED;

    if (Completed >= Value)
        return S_OK;

    HRESULT Result = m_pFence->SetEventOnCompletion(
        Value, m_FenceEvent);

    if (FAILED(Result))
        return Result;

    const DWORD WaitResult =
        WaitForSingleObject(m_FenceEvent, INFINITE);

    if (WaitResult == WAIT_FAILED)
        return HRESULT_FROM_WIN32(GetLastError());

    if (WaitResult != WAIT_OBJECT_0)
        return E_FAIL;

    return m_pFence->GetCompletedValue() == UINT64_MAX
               ? DXGI_ERROR_DEVICE_REMOVED : S_OK;
}

void GraphicDevice::Transition_BackBuffer(D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After) {
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
    m_Support = {};
    m_Desc = {};
}

HRESULT GraphicDevice::Resize_BackBuffers(uint32_t Width, uint32_t Height) {
    Release_BackBuffers();

    const UINT Flags = m_bAllowTearing
        ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    HRESULT Result = m_pSwapChain->ResizeBuffers(
        m_Desc.BackBufferCount,
        Width,
        Height,
        m_Desc.BackBufferFormat,
        Flags);

    if (FAILED(Result))
        return Result;

    m_Desc.WindowWidth = Width;
    m_Desc.WindowHeight = Height;
    m_iFrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

    // Wait_ForGPU에서 기존 프레임 작업 완료를 확인했음.
    for (auto& Frame : m_FrameList)
        Frame.FenceValue = 0;

    return Initialize_BackBuffers();
}

HRESULT GraphicDevice::Reset_CommandRecording() {
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

HRESULT GraphicDevice::End_Frame()
{
    if (!m_bInitialized || !m_bFrameStarted)
        return E_UNEXPECTED;

    Transition_BackBuffer(
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);

    HRESULT Result = m_pCommandList->Close();
    m_bFrameStarted = false;

    if (FAILED(Result))
    {
        m_bInitialized = false;
        return Result;
    }

    ID3D12CommandList* Lists[] = {m_pCommandList.Get()};
    m_pCommandQueue->ExecuteCommandLists(1, Lists);

    const UINT SyncInterval = m_Desc.EnableVSync ? 1 : 0;
    const UINT Flags = !m_Desc.EnableVSync && m_bAllowTearing
        ? DXGI_PRESENT_ALLOW_TEARING : 0;

    const HRESULT PresentResult =
        m_pSwapChain->Present(SyncInterval, Flags);

    const uint64_t FenceValue = m_iNextFenceValue++;

    Result = m_pCommandQueue->Signal(m_pFence.Get(), FenceValue);

    if (FAILED(Result))
    {
        m_bInitialized = false;
        return Result;
    }

    m_FrameList[m_iFrameIndex].FenceValue = FenceValue;

    if (FAILED(PresentResult))
        m_bInitialized = false;

    return PresentResult;
}