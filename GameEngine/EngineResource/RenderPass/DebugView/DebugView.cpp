#include "DebugView.h"

#include "GraphicDevice.h"
#include "RenderManager.h"
#include "ShaderManager.h"
ENGINE_USE

DebugView::~DebugView() { ShutDown(); }

HRESULT DebugView::Initialize(const GraphicDevice& Device, const ShaderManager& Shaders, const std::wstring& ShaderPath) {
    if (m_RootSignature || m_PipelineState) return E_UNEXPECTED;

    if (!Device.Get_Device()) return E_INVALIDARG;

    HRESULT Result = Compile_Shaders(Shaders, ShaderPath);
    
    if (SUCCEEDED(Result))  Result = Create_RootSignature(Device.Get_Device());
    if (SUCCEEDED(Result))  Result = Create_PipelineState(Device);
    if (FAILED(Result))     ShutDown();

    return Result;
}

HRESULT DebugView::Render(RenderManager& Manager) {
    if (!m_RootSignature || !m_PipelineState || !Manager.m_pGraphicDevice) return E_UNEXPECTED;

    auto& Device = *Manager.m_pGraphicDevice;
    auto* CommandList = Device.Get_CommandList();
    if (!CommandList) return E_UNEXPECTED;

    Bind_BackBuffer(CommandList, Device);

    CommandList->SetPipelineState(m_PipelineState.Get());
    CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

    const HRESULT Result = Bind_Source(Manager, CommandList);
    if (FAILED(Result)) return Result;

    CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    CommandList->DrawInstanced(3, 1, 0, 0);

    return S_OK;
}

void DebugView::ShutDown() {
    m_PipelineState.Reset();
    m_RootSignature.Reset();
    m_VertexShader.reset();
    m_PixelShader.reset();
}

HRESULT DebugView::Compile_Shaders(const ShaderManager& Shaders, const std::wstring& ShaderPath) {
    m_VertexShader  = std::make_unique<ShaderResource>(ShaderDesc{ ShaderPath, VertexShader_MainEntryPoint, VertexShader_DefaultProfile });

    HRESULT Result = Shaders.Compile_Shader(*m_VertexShader);
    if (FAILED(Result)) return Result;
    
    m_PixelShader   = std::make_unique<ShaderResource>(ShaderDesc{ ShaderPath, PixelShader_MainEntryPoint, PixelShader_DefaultProfile });
    
    Result = Shaders.Compile_Shader(*m_PixelShader);
    if (FAILED(Result)) return Result;

    return Result;
}

HRESULT DebugView::Create_RootSignature(ID3D12Device* Device) {
    CD3DX12_DESCRIPTOR_RANGE Range;
    Range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    CD3DX12_ROOT_PARAMETER Parameters[2];
    Parameters[0].InitAsDescriptorTable(1, &Range, D3D12_SHADER_VISIBILITY_PIXEL);
    Parameters[1].InitAsConstants(3, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC Sampler(
        0,
        D3D12_FILTER_MIN_MAG_MIP_POINT,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    Sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    CD3DX12_ROOT_SIGNATURE_DESC Desc;
    Desc.Init(2, Parameters, 1, &Sampler);

    ComPtr<ID3DBlob> Blob;
    ComPtr<ID3DBlob> Errors;

    HRESULT Result = D3D12SerializeRootSignature(
        &Desc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        Blob.GetAddressOf(),
        Errors.GetAddressOf());

    if (FAILED(Result)) {
        if (Errors) OutputDebugStringA(static_cast<const char*>(Errors->GetBufferPointer()));
        return Result;
    }

    return Device->CreateRootSignature(
        0,
        Blob->GetBufferPointer(),
        Blob->GetBufferSize(),
        IID_PPV_ARGS(m_RootSignature.GetAddressOf()));
}

HRESULT ENGINE::DebugView::Create_PipelineState(const GraphicDevice& Device) {
    D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc{};

    Desc.pRootSignature = m_RootSignature.Get();
    Desc.VS = m_VertexShader->Get_ByteCode();
    Desc.PS = m_PixelShader->Get_ByteCode();

    Desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    Desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    Desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    Desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    Desc.DepthStencilState.DepthEnable = FALSE;
    Desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    Desc.DepthStencilState.StencilEnable = FALSE;

    Desc.SampleMask = UINT_MAX;
    Desc.SampleDesc.Count = 1;
    Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    Desc.NumRenderTargets = 1;
    Desc.RTVFormats[0] = Device.Get_GraphicDeviceDesc().m_BackBufferFormat;

    return Device.Get_Device()->CreateGraphicsPipelineState(&Desc, IID_PPV_ARGS(m_PipelineState.GetAddressOf()));
}

void ENGINE::DebugView::Bind_BackBuffer(ID3D12GraphicsCommandList* CommandList, const GraphicDevice& Device) const {
    const auto View = Device.Get_CurrentView();

    CommandList->OMSetRenderTargets(1, &View.RTV, FALSE, nullptr);

    CommandList->RSSetViewports(1, &View.Viewport);
    CommandList->RSSetScissorRects(1, &View.ScissorRect);
}

HRESULT DebugView::Bind_Source(RenderManager& Manager, ID3D12GraphicsCommandList* CommandList) {
    D3D12_GPU_DESCRIPTOR_HANDLE SRV{};

    const auto Mode = static_cast<DebugViewMode>(m_View.Mode);

    if (Mode == DebugViewMode::Depth) {
        auto* Target = Manager.Get_DepthTarget(DepthTargetID::SceneDepth);
        if (!Target) return E_UNEXPECTED;

        Target->Transition(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        SRV = Target->Get_SRV();
    }
    else {
        RenderTargetID ID;

        switch (Mode) {
            case DebugViewMode::Albedo:     ID = RenderTargetID::Albedo;     break;
            case DebugViewMode::Normal:     ID = RenderTargetID::Normal;     break;
            case DebugViewMode::Material:   ID = RenderTargetID::Material;   break;
            case DebugViewMode::SceneColor: ID = RenderTargetID::SceneColor; break;
            default: return E_INVALIDARG;
        }

        auto* Target = Manager.Get_RenderTarget(ID);
        if (!Target) return E_UNEXPECTED;

        Target->Transition(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        SRV = Target->Get_SRV();
    }

    CommandList->SetGraphicsRootDescriptorTable(0, SRV);
    CommandList->SetGraphicsRoot32BitConstants(1, 3, &m_View, 0);

    return S_OK;
}
