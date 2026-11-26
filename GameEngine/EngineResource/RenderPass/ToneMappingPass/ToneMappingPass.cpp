#include "ToneMappingPass.h"

#include "GraphicDevice.h"
#include "RenderManager.h"
#include "ShaderManager.h"
ENGINE_USE
ToneMappingPass::~ToneMappingPass() { ShutDown(); }

HRESULT ToneMappingPass::Initialize(const GraphicDevice& Device, const ShaderManager& Shaders, const std::wstring& ShaderPath) {
    if (m_RootSignature || m_PipelineState) return E_UNEXPECTED;

    if (!Device.Get_Device()) return E_INVALIDARG;

    HRESULT Result = Compile_Shaders(Shaders, ShaderPath);

    if (SUCCEEDED(Result)) Result = Create_RootSignature(Device.Get_Device());
    if (SUCCEEDED(Result)) Result = Create_PipelineState(Device);
    if (FAILED(Result)) ShutDown();

    return Result;
}

void ToneMappingPass::ShutDown() {
    m_PipelineState.Reset();
    m_RootSignature.Reset();
    m_VertexShader.reset();
    m_PixelShader.reset();

    m_fExposure = 1.f;
}

HRESULT ToneMappingPass::Set_Exposure(float Exposure) {
    if (!std::isfinite(Exposure) || Exposure < 0.f) return E_INVALIDARG;

    m_fExposure = Exposure;
    return S_OK;
}

HRESULT ToneMappingPass::Compile_Shaders(const ShaderManager& Shaders, const std::wstring& ShaderPath) {
    m_VertexShader  = std::make_unique<ShaderResource>(ShaderDesc{ ShaderPath, VertexShader_MainEntryPoint, VertexShader_DefaultProfile });

    HRESULT Result = Shaders.Compile_Shader(*m_VertexShader);
    if (FAILED(Result)) return Result;
    
    m_PixelShader   = std::make_unique<ShaderResource>(ShaderDesc{ ShaderPath, PixelShader_MainEntryPoint, PixelShader_DefaultProfile });
    
    Result = Shaders.Compile_Shader(*m_PixelShader);
    if (FAILED(Result)) return Result;

    return Result;
}

HRESULT ToneMappingPass::Create_RootSignature(ID3D12Device* Device)
{
    CD3DX12_DESCRIPTOR_RANGE Range;
    Range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    CD3DX12_ROOT_PARAMETER Parameters[2];

    Parameters[0].InitAsDescriptorTable(1, &Range, D3D12_SHADER_VISIBILITY_PIXEL);
    Parameters[1].InitAsConstants(1, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC Sampler(
        0,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
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

HRESULT ToneMappingPass::Create_PipelineState(const GraphicDevice& Device) {
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

void ToneMappingPass::Bind_BackBuffer(ID3D12GraphicsCommandList* CommandList, const GraphicDevice& Device) const {
    const auto View = Device.Get_CurrentView();

    CommandList->OMSetRenderTargets(1, &View.RTV, FALSE, nullptr);
    CommandList->RSSetViewports(1, &View.Viewport);
    CommandList->RSSetScissorRects(1, &View.ScissorRect);
}

HRESULT ToneMappingPass::Render(RenderManager& Manager) {
    if (!m_RootSignature || !m_PipelineState || !Manager.m_pGraphicDevice) return E_UNEXPECTED;

    auto& Device = *Manager.m_pGraphicDevice;
    auto* CommandList = Device.Get_CommandList();
    auto* SceneColor = Manager.Get_RenderTarget(RenderTargetID::SceneColor);

    if (!CommandList || !SceneColor)
        return E_UNEXPECTED;

    Bind_BackBuffer(CommandList, Device);

    SceneColor->Transition(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    CommandList->SetPipelineState(m_PipelineState.Get());
    CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
    CommandList->SetGraphicsRootDescriptorTable(0, SceneColor->Get_SRV());
    CommandList->SetGraphicsRoot32BitConstants(1, 1, &m_fExposure, 0);
    CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    CommandList->DrawInstanced(3, 1, 0, 0);

    return S_OK;
}