#include "LightPass.h"

#include "GraphicDevice.h"
#include "RenderManager.h"
#include "ShaderManager.h"
ENGINE_USE

LightPass::~LightPass() { ShutDown(); }

HRESULT LightPass::Initialize(const GraphicDevice& Device, const ShaderManager& Shaders,
    const std::wstring& ShaderPath, DXGI_FORMAT OutputFormat) {
    if (m_RootSignature || m_PipelineState)
        return E_UNEXPECTED;

    if (!Device.Get_Device()) return E_INVALIDARG;

    HRESULT Result = Compile_Shaders(Shaders, ShaderPath);

    if (SUCCEEDED(Result)) Result = Create_RootSignature(Device.Get_Device());

    if (SUCCEEDED(Result)) Result = Create_PipelineState(Device, OutputFormat);

    if (FAILED(Result)) ShutDown();

    return Result;
}

void LightPass::ShutDown() {
    m_PipelineState.Reset();
    m_RootSignature.Reset();
    m_VertexShader.reset();
    m_PixelShader.reset();
}

HRESULT LightPass::Compile_Shaders(const ShaderManager& Shaders, const std::wstring& ShaderPath) {
    m_VertexShader  = std::make_unique<ShaderResource>(ShaderDesc{ ShaderPath, VertexShader_MainEntryPoint, VertexShader_DefaultProfile });

    HRESULT Result = Shaders.Compile_Shader(*m_VertexShader);
    if (FAILED(Result)) return Result;
    
    m_PixelShader   = std::make_unique<ShaderResource>(ShaderDesc{ ShaderPath, PixelShader_MainEntryPoint, PixelShader_DefaultProfile });
    
    Result = Shaders.Compile_Shader(*m_PixelShader);
    if (FAILED(Result)) return Result;

    return Result;
}

HRESULT LightPass::Create_RootSignature(ID3D12Device* Device) {
    CD3DX12_DESCRIPTOR_RANGE Ranges[4];
    CD3DX12_ROOT_PARAMETER Parameters[5];

    for (UINT i = 0; i < 4; ++i) {
        Ranges[i].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, i);

        Parameters[i].InitAsDescriptorTable(1, &Ranges[i], D3D12_SHADER_VISIBILITY_PIXEL);
    }

    Parameters[4].InitAsConstants(28, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_ROOT_SIGNATURE_DESC Desc;
    Desc.Init(5, Parameters);

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

    return Device->CreateRootSignature(0, Blob->GetBufferPointer(), 
        Blob->GetBufferSize(), IID_PPV_ARGS(m_RootSignature.GetAddressOf()));
}

HRESULT LightPass::Create_PipelineState(const GraphicDevice& Device, DXGI_FORMAT OutputFormat) {
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
    Desc.RTVFormats[0] = OutputFormat;

    return Device.Get_Device()->CreateGraphicsPipelineState(&Desc, IID_PPV_ARGS(m_PipelineState.GetAddressOf()));
}

HRESULT LightPass::Bind_Inputs(RenderManager& Manager, ID3D12GraphicsCommandList* CommandList) {
    auto* Output = Manager.Get_RenderTarget(RenderTargetID::SceneColor);
    auto* Depth = Manager.Get_DepthTarget(DepthTargetID::SceneDepth);

    if (!Output || !Depth)
        return E_UNEXPECTED;

    const auto& Size = Output->Get_RenderTargetDesc();
    const auto& DepthSize = Depth->Get_DepthTargetDesc();

    if (DepthSize.DepthTargetWidth != Size.RenderTargetWidth ||
        DepthSize.DepthTargetHeight != Size.RenderTargetHeight)
        return E_INVALIDARG;

    constexpr std::array IDs{
        RenderTargetID::Albedo,
        RenderTargetID::Normal,
        RenderTargetID::Material
    };

    for (UINT i = 0; i < IDs.size(); ++i) {
        auto* Target = Manager.Get_RenderTarget(IDs[i]);
        if (!Target)
            return E_UNEXPECTED;

        const auto& Desc = Target->Get_RenderTargetDesc();

        if (Desc.RenderTargetWidth != Size.RenderTargetWidth ||
            Desc.RenderTargetHeight != Size.RenderTargetHeight)
            return E_INVALIDARG;

        Target->Transition(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        CommandList->SetGraphicsRootDescriptorTable(i, Target->Get_SRV());
    }

    Depth->Transition(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    CommandList->SetGraphicsRootDescriptorTable(3, Depth->Get_SRV());

    return S_OK;
}

HRESULT LightPass::Render(RenderManager& Manager, const LightingConstants& Constants) {
    if (!m_RootSignature || !m_PipelineState || !Manager.m_pGraphicDevice)
        return E_UNEXPECTED;

    auto* CommandList = Manager.m_pGraphicDevice->Get_CommandList();
    if (!CommandList)
        return E_UNEXPECTED;

    constexpr std::array Outputs{RenderTargetID::SceneColor};

    // 깊이 타겟을 바인딩하지 않음.
    HRESULT Result = Manager.Begin_Pass(
        Outputs, DepthTargetID::Count);

    if (FAILED(Result))
        return Result;

    CommandList->SetPipelineState(m_PipelineState.Get());
    CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

    Result = Bind_Inputs(Manager, CommandList);
    if (FAILED(Result))
        return Result;

    CommandList->SetGraphicsRoot32BitConstants(
        4, 28, &Constants, 0);

    CommandList->IASetPrimitiveTopology(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    CommandList->DrawInstanced(3, 1, 0, 0);

    return S_OK;
}
