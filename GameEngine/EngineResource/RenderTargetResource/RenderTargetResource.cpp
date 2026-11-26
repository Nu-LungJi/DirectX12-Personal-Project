#include "RenderTargetResource.h"
ENGINE_USE

RenderTargetResource::~RenderTargetResource() { ShutDown(); }

HRESULT RenderTargetResource::Initialize(GraphicDevice& Device, const RenderTargetDesc& Desc) {
    if (m_pResource)         return E_UNEXPECTED;
    
    if (m_pGraphicDevice)       return E_UNEXPECTED;

    if (!Device.Get_Device())   return E_INVALIDARG;

    m_pGraphicDevice = &Device;
    m_Desc = Desc;

    HRESULT Result = Validate_Desc();

    if (SUCCEEDED(Result)) Result = Create_Resource();
    if (SUCCEEDED(Result)) Result = Create_Views();
    if (FAILED(Result)) ShutDown();

    return Result;
    
}

void RenderTargetResource::Transition(D3D12_RESOURCE_STATES State) {
    if (!m_pGraphicDevice || !m_pResource || m_ResourceState == State) return;

    auto* CommandList = m_pGraphicDevice->Get_CommandList();
    if (!CommandList) return;

    const auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_pResource.Get(), m_ResourceState, State);

    CommandList->ResourceBarrier(1, &Barrier);
    m_ResourceState = State;
}

void RenderTargetResource::Clear() {
    if (!m_pGraphicDevice || !m_pResource) return;

    auto* CommandList = m_pGraphicDevice->Get_CommandList();
    if (!CommandList) return;

    Transition(D3D12_RESOURCE_STATE_RENDER_TARGET);

    CommandList->ClearRenderTargetView(m_RTV.CPU, m_Desc.ClearColor, 0, nullptr);
}

void RenderTargetResource::ShutDown() {
    m_pResource.Reset();

    if (m_pGraphicDevice) {
        if (m_RTV.Index != UINT32_MAX) m_pGraphicDevice->Get_RTVAllocator().Free(m_RTV);
        if (m_SRV.Index != UINT32_MAX) m_pGraphicDevice->Get_ResourceAllocator().Free(m_SRV);
    }

    m_RTV = {}; m_SRV = {}; m_Desc = {};
    m_pGraphicDevice = nullptr;
    m_ResourceState = D3D12_RESOURCE_STATE_COMMON;
}

HRESULT RenderTargetResource::Validate_Desc() const {
    if (m_Desc.RenderTargetWidth == 0 || m_Desc.RenderTargetHeight == 0 ||
        m_Desc.RenderTargetWidth  > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION ||
        m_Desc.RenderTargetHeight > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION)
        return E_INVALIDARG;

    D3D12_FEATURE_DATA_FORMAT_SUPPORT Support{};
    Support.Format = m_Desc.RenderTargetFormat;

    HRESULT Result = m_pGraphicDevice->Get_Device()->CheckFeatureSupport(
        D3D12_FEATURE_FORMAT_SUPPORT, &Support, sizeof(Support));
    if (FAILED(Result)) return Result;

    constexpr UINT Required = D3D12_FORMAT_SUPPORT1_TEXTURE2D | D3D12_FORMAT_SUPPORT1_RENDER_TARGET | D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE;

    return (Support.Support1 & Required) == Required ? S_OK : E_INVALIDARG;
}

HRESULT RenderTargetResource::Create_Resource() {
    const auto Heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    const auto Desc = CD3DX12_RESOURCE_DESC::Tex2D(m_Desc.RenderTargetFormat, m_Desc.RenderTargetWidth, m_Desc.RenderTargetHeight,
        1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    D3D12_CLEAR_VALUE ClearValue{};
    ClearValue.Format = m_Desc.RenderTargetFormat;
    
    for (uint32_t i = 0; i < 4; ++i)
        ClearValue.Color[i] = m_Desc.ClearColor[i];

    return m_pGraphicDevice->Get_Device()->CreateCommittedResource(&Heap, D3D12_HEAP_FLAG_NONE, &Desc,
        D3D12_RESOURCE_STATE_COMMON, &ClearValue, IID_PPV_ARGS(m_pResource.GetAddressOf()));
}

HRESULT RenderTargetResource::Create_Views() {
    HRESULT Result = m_pGraphicDevice->Get_RTVAllocator().Allocate(m_RTV);
    if (FAILED(Result)) return Result;

    Result = m_pGraphicDevice->Get_ResourceAllocator().Allocate(m_SRV);
    if (FAILED(Result)) return Result;

    auto* Device = m_pGraphicDevice->Get_Device();

    D3D12_RENDER_TARGET_VIEW_DESC RTVDesc{};
    RTVDesc.Format          = m_Desc.RenderTargetFormat;
    RTVDesc.ViewDimension   = D3D12_RTV_DIMENSION_TEXTURE2D;

    Device->CreateRenderTargetView(m_pResource.Get(), &RTVDesc, m_RTV.CPU);

    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
    SRVDesc.Format          = m_Desc.RenderTargetFormat;
    SRVDesc.ViewDimension   = D3D12_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SRVDesc.Texture2D.MipLevels = 1;

    Device->CreateShaderResourceView(m_pResource.Get(), &SRVDesc, m_SRV.CPU);

    return S_OK;
}
