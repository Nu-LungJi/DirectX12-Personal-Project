#include "RenderManager.h"

#include "MeshResource.h"
#include "ShaderManager.h"
#include "ToneMappingPass.h"
ENGINE_USE

HRESULT GeometryPass::Initialize(class RenderManager& Manager, const class ShaderManager& Shaders, const std::wstring& ShaderPath) {
    if (m_RootSignature || m_PipelineState) return E_UNEXPECTED;

    if (!Manager.m_pGraphicDevice || !Manager.m_pGraphicDevice->Get_Device()) return E_UNEXPECTED;

    HRESULT Result = Compile_Shaders(Shaders, ShaderPath);
    if (SUCCEEDED(Result)) Result = Create_RootSignature(Manager.m_pGraphicDevice->Get_Device());
    if (SUCCEEDED(Result)) Result = Create_PipelineState(Manager);
    if (SUCCEEDED(Result)) Result = Initialize_TransformBuffer(*Manager.m_pGraphicDevice);
    
    if (FAILED(Result)) ShutDown();
    
    return Result;
}

HRESULT GeometryPass::Render(class RenderManager& RM, std::span<const GeometryDrawItem> Items) {
    if (!m_RootSignature || !m_PipelineState || !RM.m_pGraphicDevice)
        return E_UNEXPECTED;

    auto& Device = *RM.m_pGraphicDevice;
    auto* CommandList = RM.m_pGraphicDevice->Get_CommandList();
    if (!CommandList) return E_UNEXPECTED;
    
    const uint32_t FrameIndex = Device.Get_FrameIndex();
    
    HRESULT Result = Prepare_Draws(FrameIndex, Items);
    if (FAILED(Result)) return Result;
    
    Result = RM.Begin_Pass(ColorTargets, DepthTargetID::SceneDepth);
    if (FAILED(Result)) return Result;

    CommandList->SetPipelineState(m_PipelineState.Get());
    CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
    CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    const uint32_t BaseSlot = FrameIndex * MaxGeometryDrawCount;

    for (uint32_t i = 0; i < Items.size(); ++i) {
        Bind_Transform(CommandList, BaseSlot + i);
        Bind_Mesh(CommandList, *Items[i].Mesh);
        Draw_Mesh(CommandList, *Items[i].Mesh);
    }
    
    return S_OK;
}

void GeometryPass::ShutDown() {
    m_TransformBuffer.ShutDown();
    
    m_PipelineState .Reset();
    m_RootSignature .Reset();
    m_VertexShader  .reset();
    m_PixelShader   .reset();
}

HRESULT GeometryPass::Compile_Shaders(const ShaderManager& Shaders, const std::wstring& ShaderPath) {
    m_VertexShader  = std::make_unique<ShaderResource>(ShaderDesc{ ShaderPath, VertexShader_MainEntryPoint, VertexShader_DefaultProfile });

    HRESULT Result = Shaders.Compile_Shader(*m_VertexShader);
    if (FAILED(Result)) return Result;
    
    m_PixelShader   = std::make_unique<ShaderResource>(ShaderDesc{ ShaderPath, PixelShader_MainEntryPoint, PixelShader_DefaultProfile });
    
    Result = Shaders.Compile_Shader(*m_PixelShader);
    if (FAILED(Result)) return Result;

    return Result;
}

HRESULT GeometryPass::Create_RootSignature(ID3D12Device* Device) {
    CD3DX12_ROOT_PARAMETER Parameter;
    Parameter.InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    CD3DX12_ROOT_SIGNATURE_DESC Desc;
    Desc.Init(1, &Parameter, 0, nullptr, 
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> Blob;
    ComPtr<ID3DBlob> Errors;

    HRESULT Result = D3D12SerializeRootSignature(&Desc, D3D_ROOT_SIGNATURE_VERSION_1, 
        Blob.GetAddressOf(), Errors.GetAddressOf());

    if (FAILED(Result)) {
        if (Errors) OutputDebugStringA(static_cast<const char*>(Errors->GetBufferPointer()));
        return Result;
    }

    return Device->CreateRootSignature(0, Blob->GetBufferPointer(),
        Blob->GetBufferSize(), IID_PPV_ARGS(m_RootSignature.GetAddressOf()));
}

HRESULT GeometryPass::Create_PipelineState(RenderManager& Manager) {
    constexpr D3D12_INPUT_ELEMENT_DESC InputLayout[] = {
        {
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
            static_cast<UINT>(offsetof(Vertex, Position)),
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
        {
            "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
            static_cast<UINT>(offsetof(Vertex, Normal)),
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
        {
            "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
            static_cast<UINT>(offsetof(Vertex, TexCoord)),
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc{};
    Desc.pRootSignature = m_RootSignature.Get();
    Desc.VS = m_VertexShader->Get_ByteCode();
    Desc.PS = m_PixelShader->Get_ByteCode();

    Desc.InputLayout = {
        InputLayout,
        static_cast<UINT>(_countof(InputLayout))
    };

    Desc.BlendState                  = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    Desc.RasterizerState             = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    Desc.DepthStencilState           = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    Desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

    Desc.SampleMask                  = UINT_MAX;
    Desc.SampleDesc.Count            = 1;
    Desc.PrimitiveTopologyType       = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    Desc.NumRenderTargets            = static_cast<UINT>(ColorTargets.size());

    for (UINT i = 0; i < Desc.NumRenderTargets; ++i) {
        const auto* Target = Manager.Get_RenderTarget(ColorTargets[i]);
        if (!Target) return E_UNEXPECTED;

        Desc.RTVFormats[i] = Target->Get_RenderTargetDesc().RenderTargetFormat;
    }

    if (!Manager.Get_DepthTarget(DepthTargetID::SceneDepth))
        return E_UNEXPECTED;

    // 현재 DepthTargetResource의 DSV 포맷.
    Desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

    return Manager.m_pGraphicDevice->Get_Device()->CreateGraphicsPipelineState(&Desc,
            IID_PPV_ARGS(m_PipelineState.GetAddressOf()));
}

HRESULT GeometryPass::Initialize_TransformBuffer(const GraphicDevice& Device) {
    const uint32_t SlotCount = static_cast<uint32_t>(Device.Get_GraphicDeviceDesc().m_iBackBufferCount) * MaxGeometryDrawCount;
    
    return m_TransformBuffer.Initialize(Device.Get_Device(), static_cast<uint32_t>(sizeof(TransformConstants)), SlotCount);
}

HRESULT GeometryPass::Upload_Transform(uint32_t SlotIndex, const TransformConstants& Transform) {
    return m_TransformBuffer.Write(SlotIndex, &Transform, static_cast<uint32_t>(sizeof(TransformConstants)));
}

void GeometryPass::Bind_Transform(ID3D12GraphicsCommandList* CommandList, uint32_t SlotIndex) const{
    CommandList->SetGraphicsRootConstantBufferView(0, m_TransformBuffer.Get_GPUAddress(SlotIndex));
}

HRESULT GeometryPass::Prepare_Draws(uint32_t SlotIndex, std::span<const GeometryDrawItem> Items) {
    if (Items.size() > MaxGeometryDrawCount) return E_INVALIDARG;

    const uint32_t BaseSlot = SlotIndex * MaxGeometryDrawCount;

    for (uint32_t i = 0; i < Items.size(); ++i) {
        const auto& Item = Items[i];
        if (!Item.Mesh) return E_INVALIDARG;

        HRESULT Result = Validate_Mesh(*Item.Mesh);
        if (FAILED(Result)) return Result;

        Result = Upload_Transform(BaseSlot + i, Item.Transform);
        if (FAILED(Result)) return Result;
    }

    return S_OK;
}

HRESULT GeometryPass::Validate_Mesh(const MeshResource& Mesh) const {
    const auto VertexView = Mesh.Get_VertexBufferView();

    if (Mesh.Get_VertexCount() == 0 || VertexView.BufferLocation == 0 || VertexView.StrideInBytes != sizeof(Vertex))
        return E_INVALIDARG;

    if (Mesh.Get_IndexCount() > 0) {
        const auto IndexView = Mesh.Get_IndexBufferView();

        if (IndexView.BufferLocation == 0 || IndexView.Format != DXGI_FORMAT_R32_UINT)
            return E_INVALIDARG;
    }

    return S_OK;
}

void GeometryPass::Bind_Mesh(ID3D12GraphicsCommandList* CommandList, const MeshResource& Mesh) const {
    const auto VertexView = Mesh.Get_VertexBufferView();
    CommandList->IASetVertexBuffers(0, 1, &VertexView);

    if (Mesh.Get_IndexCount() > 0) {
        const auto IndexView = Mesh.Get_IndexBufferView();
        CommandList->IASetIndexBuffer(&IndexView);
    }
    else {
        CommandList->IASetIndexBuffer(nullptr);
    }
}

void GeometryPass::Draw_Mesh(ID3D12GraphicsCommandList* CommandList, const MeshResource& Mesh) const {
    if (Mesh.Get_IndexCount() > 0) 
        CommandList->DrawIndexedInstanced(Mesh.Get_IndexCount(), 1, 0, 0, 0);
    else {
        CommandList->DrawInstanced(Mesh.Get_VertexCount(), 1, 0, 0);
    }
}

RenderManager::~RenderManager() { ShutDown(); }

HRESULT RenderManager::Initialize(GraphicDevice& Device, const ShaderManager& Shaders, const std::wstring& GeometryShaderPath) {
    if (m_pGraphicDevice)     return E_UNEXPECTED;

    if (!Device.Get_Device()) return E_INVALIDARG;

    m_pGraphicDevice = &Device;
    
    HRESULT Result = Initialize_DefaultTargets();
    
    if (SUCCEEDED(Result)) Result = m_GeometryPass.Initialize(*this, Shaders, GeometryShaderPath);
    if (SUCCEEDED(Result)) {
        auto Path = std::filesystem::path(GeometryShaderPath);
        Path.replace_filename(L"DebugView.hlsl");

        Result = m_DebugViewPass.Initialize(Device, Shaders, Path.wstring());
    }
    
    if (SUCCEEDED(Result)) {
        auto Path = std::filesystem::path(GeometryShaderPath);
        Path.replace_filename(L"LightPass.hlsl");

        const auto* Target = Get_RenderTarget(RenderTargetID::SceneColor);

        Result = Target ? m_LightingPass.Initialize(Device, Shaders, Path.wstring(),
                Target->Get_RenderTargetDesc().RenderTargetFormat) : E_UNEXPECTED;
    }
    
    if (SUCCEEDED(Result)) {
        auto Path = std::filesystem::path(GeometryShaderPath);
        Path.replace_filename(L"ToneMapping.hlsl");

        Result = m_ToneMappingPass.Initialize(Device, Shaders, Path.wstring());
    }
    
    if (SUCCEEDED(Result))
        m_DebugViewPass.Set_Mode(DebugViewMode::SceneColor);
    
    if (FAILED(Result)) ShutDown();

    return Result;
}

HRESULT RenderManager::Initialize_DefaultTargets() {
    struct TargetDefinition {
        RenderTargetID ID;
        DXGI_FORMAT Format;
    };

    constexpr TargetDefinition Targets[] = {
        { RenderTargetID::Albedo,     DXGI_FORMAT_R8G8B8A8_UNORM },
        { RenderTargetID::Normal,     DXGI_FORMAT_R16G16B16A16_FLOAT },
        { RenderTargetID::Material,   DXGI_FORMAT_R8G8B8A8_UNORM },
        { RenderTargetID::SceneColor, DXGI_FORMAT_R16G16B16A16_FLOAT }
    };

    for (const auto& Target : Targets) {
        RenderTargetRegistration Desc{};
        Desc.m_RenderTargetDesc.RenderTargetFormat = Target.Format;

        const HRESULT Result = Register_RenderTarget(Desc, Target.ID);
        if (FAILED(Result)) return Result;
    }

    DepthTargetRegistration DepthDesc{};
    return Register_DepthTarget(DepthDesc, DepthTargetID::SceneDepth);
}

void RenderManager::Update() {
    
}

HRESULT RenderManager::Render() {
    //if (!m_pGraphicDevice || !m_pGraphicDevice->Get_CommandList()) return E_UNEXPECTED;
    
    //const HRESULT Result = m_GeometryPass.Render(*this, std::span<const GeometryDrawItem>(m_DrawItems.data(), m_DrawCount));
    
    //Clear_DrawList();
    
    //if (FAILED(Result)) return Result;

    //return m_DebugViewPass.Render(*this);
    
    if (!m_pGraphicDevice || !m_pGraphicDevice->Get_CommandList()) return E_UNEXPECTED;

    HRESULT Result = m_GeometryPass.Render(*this, std::span<const GeometryDrawItem>(m_DrawItems.data(), m_DrawCount));

    Clear_DrawList();

    if (FAILED(Result)) return Result;

    Result = m_LightingPass.Render(*this, m_LightingConstants);
    if (FAILED(Result)) return Result;

    if (m_DebugViewEnabled)
        return m_DebugViewPass.Render(*this);
    
    return m_ToneMappingPass.Render(*this);
}

HRESULT RenderManager::Resolve_TargetSize(const TargetSizeDesc& Desc, uint32_t& Width, uint32_t& Height) const {
    double TargetWidth = 0;
    double TargetHeight = 0;
    
    switch (Desc.m_Policy) {
    case TargetSizePolicy::Fixed:
        {
            TargetWidth = Desc.m_iWidth;
            TargetHeight = Desc.m_iHeight;
            break;
        }
        

    case TargetSizePolicy::Relative:
        {
            if (!m_pGraphicDevice || !m_pGraphicDevice->Get_Device())
                return E_UNEXPECTED;

            if (!std::isfinite(Desc.m_fRenderTargetScale) || Desc.m_fRenderTargetScale <= 0.f)
                return E_INVALIDARG;

            const auto& DeviceDesc = m_pGraphicDevice->Get_GraphicDeviceDesc();

            if (DeviceDesc.m_iWindowWidth == 0 || DeviceDesc.m_iWindowHeight == 0)
                return E_INVALIDARG;

            TargetWidth  = std::ceil(static_cast<double>(DeviceDesc.m_iWindowWidth) * Desc.m_fRenderTargetScale);
            TargetHeight = std::ceil(static_cast<double>(DeviceDesc.m_iWindowHeight) * Desc.m_fRenderTargetScale);
            
            break;
        }

    default: return E_INVALIDARG;
    }

    constexpr double MaxSize = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;

    if (TargetWidth < 1 || TargetHeight < 1 || TargetWidth > MaxSize || TargetHeight > MaxSize)
        return E_INVALIDARG;

    Width   = static_cast<uint32_t>(TargetWidth);
    Height  = static_cast<uint32_t>(TargetHeight);

    return S_OK;
}

HRESULT RenderManager::Create_RenderTarget(RenderTargetEntry& Entry) const {
    if (!m_pGraphicDevice || !m_pGraphicDevice->Get_Device()) return E_UNEXPECTED;

    if (Entry.m_pRenderTargetResource) return E_UNEXPECTED;

    auto Desc = Entry.m_Desc.m_RenderTargetDesc;

    HRESULT Result = Resolve_TargetSize(Entry.m_Desc.m_SizeDesc, Desc.RenderTargetWidth, Desc.RenderTargetHeight);
    if (FAILED(Result)) return Result;

    auto Resource = std::make_unique<RenderTargetResource>();

    Result = Resource->Initialize(*m_pGraphicDevice, Desc);
    if (FAILED(Result)) return Result;

    Entry.m_pRenderTargetResource = std::move(Resource);
    
    return S_OK;
}

HRESULT RenderManager::Create_DepthTarget(DepthTargetEntry& Entry) const {
    if (!m_pGraphicDevice || !m_pGraphicDevice->Get_Device()) return E_UNEXPECTED;

    if (Entry.m_pDepthTargetResource) return E_UNEXPECTED;

    auto Desc = Entry.m_Desc.m_RenderTargetDesc;

    HRESULT Result = Resolve_TargetSize(Entry.m_Desc.m_SizeDesc, Desc.DepthTargetWidth, Desc.DepthTargetHeight);
    if (FAILED(Result))return Result;

    auto Resource = std::make_unique<DepthTargetResource>();
    
    Result = Resource->Initialize(*m_pGraphicDevice, Desc);
    if (FAILED(Result)) return Result;

    Entry.m_pDepthTargetResource = std::move(Resource);

    return S_OK;   
}

HRESULT RenderManager::Begin_Pass(std::span<const RenderTargetID> ColorTargets, DepthTargetID DepthTarget, bool Clear) {
    if (!m_pGraphicDevice) return E_UNEXPECTED;

    auto* CommandList = m_pGraphicDevice->Get_CommandList();
    if (!CommandList) return E_UNEXPECTED;

    PassTargets Targets{};

    const HRESULT Result = Resolve_PassTargets(ColorTargets, DepthTarget, Targets);
    if (FAILED(Result)) return Result;

    Bind_PassTargets(CommandList, Targets);

    if (Clear) Clear_PassTargets(Targets);

    return S_OK;
}

HRESULT RenderManager::Resolve_PassTargets(std::span<const RenderTargetID> ColorTargets, DepthTargetID DepthTarget, PassTargets& Targets) const {
    if (ColorTargets.size() > Targets.Colors.size()) return E_INVALIDARG;

    const auto MatchSize = [&Targets](uint32_t Width, uint32_t Height) {
        if (Width == 0 || Height == 0) return false;

        if (Targets.Width == 0) {
            Targets.Width = Width;
            Targets.Height = Height;
        }

        return Targets.Width == Width && Targets.Height == Height;
    };

    for (const auto ID : ColorTargets) {
        if (static_cast<size_t>(ID) >= m_RenderTargetList.size()) return E_INVALIDARG;

        auto* Resource = Get_RenderTarget(ID);
        if (!Resource) return E_UNEXPECTED;

        for (uint32_t i = 0; i < Targets.ColorCount; ++i)
            if (Targets.Colors[i] == Resource) return E_INVALIDARG;

        const auto& Desc = Resource->Get_RenderTargetDesc();
        if (!MatchSize(Desc.RenderTargetWidth, Desc.RenderTargetHeight))
            return E_INVALIDARG;

        Targets.Colors[Targets.ColorCount++] = Resource;
    }

    if (DepthTarget != DepthTargetID::Count) {
        if (static_cast<size_t>(DepthTarget) >= m_DepthTargetList.size())
            return E_INVALIDARG;

        Targets.Depth = Get_DepthTarget(DepthTarget);
        if (!Targets.Depth) return E_UNEXPECTED;

        const auto& Desc = Targets.Depth->Get_DepthTargetDesc();
        if (!MatchSize(Desc.DepthTargetWidth, Desc.DepthTargetHeight))
            return E_INVALIDARG;
    }

    return Targets.Width != 0 ? S_OK : E_INVALIDARG;
}

void RenderManager::Bind_PassTargets(ID3D12GraphicsCommandList* CommandList, const PassTargets& Targets) {
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT> RTVs{};

    for (uint32_t i = 0; i < Targets.ColorCount; ++i) {
        Targets.Colors[i]->Transition(D3D12_RESOURCE_STATE_RENDER_TARGET);
        RTVs[i] = Targets.Colors[i]->Get_RTV();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DSV{};

    if (Targets.Depth) {
        Targets.Depth->Transition(D3D12_RESOURCE_STATE_DEPTH_WRITE);
        DSV = Targets.Depth->Get_DSV();
    }

    CommandList->OMSetRenderTargets(Targets.ColorCount, Targets.ColorCount ? RTVs.data() : nullptr,
        FALSE, Targets.Depth ? &DSV : nullptr);

    const D3D12_VIEWPORT Viewport{
        0.f, 0.f,
        static_cast<float>(Targets.Width),
        static_cast<float>(Targets.Height),
        0.f, 1.f
    };

    const D3D12_RECT Scissor{
        0, 0,
        static_cast<LONG>(Targets.Width),
        static_cast<LONG>(Targets.Height)
    };

    CommandList->RSSetViewports(1, &Viewport);
    CommandList->RSSetScissorRects(1, &Scissor);
}

void RenderManager::Clear_PassTargets(const PassTargets& Targets) {
    for (uint32_t i = 0; i < Targets.ColorCount; ++i) Targets.Colors[i]->Clear();
    
    if (Targets.Depth) Targets.Depth->Clear();
}

HRESULT RenderManager::Register_RenderTarget(const RenderTargetRegistration& Desc, RenderTargetID ID) {
    if (!m_pGraphicDevice || !m_pGraphicDevice->Get_Device()) return E_UNEXPECTED;
    
    const size_t Index = static_cast<size_t>(ID);
    
    if (Index >= m_RenderTargetList.size()) return E_INVALIDARG;
    
    // When Resource is already exist, returns unexpected.
    if (m_RenderTargetList[Index].m_pRenderTargetResource)
        return E_UNEXPECTED;
    
    RenderTargetEntry Entry{};
    Entry.m_Desc = Desc;
    
    // Create, Register RenderTarget on RTVList.
    HRESULT Result = Create_RenderTarget(Entry);
    if (FAILED(Result)) return Result;
    
    m_RenderTargetList[Index] = std::move(Entry);
    
    return S_OK;
}

HRESULT RenderManager::Register_DepthTarget(const DepthTargetRegistration& Desc, DepthTargetID ID) {
    if (!m_pGraphicDevice || !m_pGraphicDevice->Get_Device()) return E_UNEXPECTED;
    
    const size_t Index = static_cast<size_t>(ID);

    if (Index >= m_DepthTargetList.size()) return E_INVALIDARG;
    
    // When Resource is already exist, returns unexpected.
    if (m_DepthTargetList[Index].m_pDepthTargetResource)
        return E_UNEXPECTED;
    
    DepthTargetEntry Entry{};
    Entry.m_Desc = Desc;
    
    // Create, Register DepthTarget on DSVList.
    HRESULT Result = Create_DepthTarget(Entry);
    if (FAILED(Result)) return Result;
    
    m_DepthTargetList[Index] = std::move(Entry);
    
    return S_OK;
}

RenderTargetResource* RenderManager::Get_RenderTarget(const RenderTargetID ID) const {
    const size_t Index = static_cast<size_t>(ID);

    return Index < m_RenderTargetList.size() ? m_RenderTargetList[Index].m_pRenderTargetResource.get() : nullptr;
}

DepthTargetResource*  RenderManager::Get_DepthTarget(const DepthTargetID ID) const {
    const size_t Index = static_cast<size_t>(ID);

    return Index < m_DepthTargetList.size() ? m_DepthTargetList[Index].m_pDepthTargetResource.get() : nullptr;
}

HRESULT RenderManager::Submit_Mesh(const MeshResource& Mesh, const TransformConstants& Transform) {
    if (!m_pGraphicDevice) return E_UNEXPECTED;

    if (m_DrawCount >= MaxGeometryDrawCount) return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

    m_DrawItems[m_DrawCount++] = GeometryDrawItem{&Mesh, Transform};

    return S_OK;
}

void RenderManager::ShutDown() {
    m_GeometryPass.ShutDown();
    m_DebugViewPass.ShutDown();
    m_LightingPass.ShutDown();
    m_ToneMappingPass.ShutDown();
    m_DebugViewEnabled = false;
    Clear_DrawList();
    
    for (auto& Entry : m_RenderTargetList)  Entry = {};
    for (auto& Entry : m_DepthTargetList)   Entry = {};
    
    m_pGraphicDevice = nullptr;
}

HRESULT RenderManager::Resize() const {
    if (!m_pGraphicDevice || !m_pGraphicDevice->Get_Device()) return E_UNEXPECTED;
    
    for (auto& Entry : m_RenderTargetList) {
        // When RTV Size Policy is Fixed(not relative on Screen), Leave it undisturbed.
        if (!Entry.m_pRenderTargetResource || Entry.m_Desc.m_SizeDesc.m_Policy == TargetSizePolicy::Fixed)
            continue;
        
        uint32_t Width = 0, Height = 0;
        HRESULT Result = Resolve_TargetSize(Entry.m_Desc.m_SizeDesc, Width, Height);
        if (FAILED(Result)) return Result;
        
        auto& Resource = *Entry.m_pRenderTargetResource;
        const auto& Current = Resource.Get_RenderTargetDesc();
        
        // When Screen Size has not changed, RTV will not be changed.
        if (Current.RenderTargetWidth == Width && Current.RenderTargetHeight == Height) continue;
        
        // Regenerate RenderTarget with new RTV Size.
        auto Desc = Entry.m_Desc.m_RenderTargetDesc;
        Desc.RenderTargetWidth  = Width;
        Desc.RenderTargetHeight = Height;

        Resource.ShutDown();

        Result = Resource.Initialize(*m_pGraphicDevice, Desc);
        if (FAILED(Result)) return Result;
    }
    
    for (auto& Entry : m_DepthTargetList) {
        // When DSV Size Policy is Fixed(not relative on Screen), Leave it undisturbed.
        if (!Entry.m_pDepthTargetResource || Entry.m_Desc.m_SizeDesc.m_Policy == TargetSizePolicy::Fixed)
            continue;
        
        uint32_t Width = 0, Height = 0;
        HRESULT Result = Resolve_TargetSize(Entry.m_Desc.m_SizeDesc, Width, Height);
        if (FAILED(Result)) return Result;
        
        auto& Resource = *Entry.m_pDepthTargetResource;
        const auto& Current = Resource.Get_DepthTargetDesc();
        
        // When Screen Size has not changed, DSV will not be changed.
        if (Current.DepthTargetWidth == Width && Current.DepthTargetHeight == Height) continue;
        
        // Regenerate RenderTarget with new DSV Size.
        auto Desc = Entry.m_Desc.m_RenderTargetDesc;
        Desc.DepthTargetWidth  = Width;
        Desc.DepthTargetHeight = Height;

        Resource.ShutDown();

        Result = Resource.Initialize(*m_pGraphicDevice, Desc);
        if (FAILED(Result)) return Result;
    }
    
    return S_OK;
}