#include "DepthTargetResource.h"
ENGINE_USE

DepthTargetResource::~DepthTargetResource() { ShutDown(); }

HRESULT DepthTargetResource::Initialize(GraphicDevice& Device, const DepthTargetDesc& Desc) {
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

void DepthTargetResource::Transition(D3D12_RESOURCE_STATES State) {
    if (!m_pGraphicDevice || !m_pResource || m_ResourceState == State) return;

    auto* CommandList = m_pGraphicDevice->Get_CommandList();
    if (!CommandList) return;

    const auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_pResource.Get(), m_ResourceState, State);

    CommandList->ResourceBarrier(1, &Barrier);
    m_ResourceState = State;
}

void DepthTargetResource::Clear() {
    if (!m_pGraphicDevice || !m_pResource) return;

    auto* CommandList = m_pGraphicDevice->Get_CommandList();
    if (!CommandList) return;

    Transition(D3D12_RESOURCE_STATE_DEPTH_WRITE);

    CommandList->ClearDepthStencilView(m_DSV.CPU, D3D12_CLEAR_FLAG_DEPTH, m_Desc.ClearDepth,
        0, 0, nullptr);
}

void DepthTargetResource::ShutDown() {
    m_pResource.Reset();

    if (m_pGraphicDevice) {
        if (m_DSV.Index != UINT32_MAX) m_pGraphicDevice->Get_DSVAllocator().Free(m_DSV);
        if (m_SRV.Index != UINT32_MAX) m_pGraphicDevice->Get_ResourceAllocator().Free(m_SRV);
    }

    m_DSV = {}; m_SRV = {}; m_Desc = {};
    m_pGraphicDevice = nullptr;
    m_ResourceState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
}

HRESULT DepthTargetResource::Validate_Desc() const {
    if (m_Desc.DepthTargetWidth == 0 || m_Desc.DepthTargetHeight == 0   ||
        m_Desc.DepthTargetWidth > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION  ||
        m_Desc.DepthTargetHeight > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION ||
        !std::isfinite(m_Desc.ClearDepth) || m_Desc.ClearDepth < 0.f || m_Desc.ClearDepth > 1.f)
        return E_INVALIDARG;

    return S_OK;
}

HRESULT DepthTargetResource::Create_Resource() {
    const auto Heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    const auto Desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS, m_Desc.DepthTargetWidth, m_Desc.DepthTargetHeight,
        1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    D3D12_CLEAR_VALUE ClearValue{};
    ClearValue.Format               = DXGI_FORMAT_D32_FLOAT;
    ClearValue.DepthStencil.Depth   = m_Desc.ClearDepth;

    m_ResourceState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    return m_pGraphicDevice->Get_Device()->CreateCommittedResource(&Heap, D3D12_HEAP_FLAG_NONE, &Desc,
    m_ResourceState, &ClearValue, IID_PPV_ARGS(m_pResource.GetAddressOf()));
}

HRESULT DepthTargetResource::Create_Views() {
    HRESULT Result = m_pGraphicDevice->Get_DSVAllocator().Allocate(m_DSV);
    if (FAILED(Result)) return Result;

    Result = m_pGraphicDevice->Get_ResourceAllocator().Allocate(m_SRV);
    if (FAILED(Result)) return Result;

    auto* Device = m_pGraphicDevice->Get_Device();

    D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc{};
    DSVDesc.Format              = DXGI_FORMAT_D32_FLOAT;
    DSVDesc.ViewDimension       = D3D12_DSV_DIMENSION_TEXTURE2D;

    Device->CreateDepthStencilView(m_pResource.Get(), &DSVDesc, m_DSV.CPU);

    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
    SRVDesc.Format              = DXGI_FORMAT_R32_FLOAT;
    SRVDesc.ViewDimension       = D3D12_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SRVDesc.Texture2D.MipLevels = 1;

    Device->CreateShaderResourceView(m_pResource.Get(), &SRVDesc, m_SRV.CPU);

    return S_OK;
}
