#include "GameInstance.h"
#include "GraphicDevice.h"
#include "TimeManager.h"
#include "SoundManager.h"
#include "GameController.h"
#include "MeshResource.h"
ENGINE_USE

GameInstance::~GameInstance()
{
    
    ShutDown();
}

HRESULT GameInstance::Initialize(const GraphicDeviceDesc& GraphicDesc) {
    HRESULT Result = m_pGraphicDevice.Initialize(GraphicDesc);
    if (FAILED(Result)) return Result;
    
    Result = Update_CameraProjection();
    if (FAILED(Result)) return Result;
    
    Result = m_pGameController.Initialize();
    if (FAILED(Result)) return Result;
        
    Result = m_pShaderManager.Initialize();
    if (FAILED(Result)) return Result;
    
    Result = m_pSoundManager.Initialize();
    if (FAILED(Result)) return Result;
    
    Result = m_pRenderManager.Initialize(m_pGraphicDevice, m_pShaderManager, 
    L"C:/Users/Yun/Desktop/DirectX12-Personal-Project/GameEngine/EngineShader/Geometry.hlsl");
    if (FAILED(Result)) return Result;
    
    m_pRenderManager.Set_DebugView(DebugViewMode::Albedo);
    
    

    
    m_pTimeManager.Reset();
    
    return S_OK;
}

void GameInstance::Update() {
    const bool ResetTime = m_pTimeResetRequested.exchange(false, std::memory_order_acq_rel);
    ResetTime ? m_pTimeManager.Reset() : m_pTimeManager.Update();

    m_pGameController.Begin_Frame();
    m_pShaderManager.Update();
    m_pSoundManager.Update();
}

void GameInstance::LateUpdate() {
    
}

HRESULT GameInstance::Render() noexcept {
    if (m_bResizePending)
    {
        m_bResizePending = false;

        HRESULT Result = m_pGraphicDevice.Resize(m_iPendingWidth, m_iPendingHeight);
        if (Result != S_OK) return Result;
        
        Result = m_pRenderManager.Resize();
        if (FAILED(Result)) return Result;
        
        Result = Update_CameraProjection();
        if (FAILED(Result)) return Result;
    }

    HRESULT Result = m_pGraphicDevice.Begin_Frame();
    if (Result != S_OK) return Result;
    
    Result = Initialize_TestMesh();
    if (FAILED(Result)) return Result;
    
    m_pRenderManager.Clear_DrawList();

    Result = Submit_TestMeshes();
    if (FAILED(Result)) return Result;
    
    auto* CommandList = m_pGraphicDevice.Get_CommandList();
    const auto View = m_pGraphicDevice.Get_CurrentView();

    CommandList->RSSetViewports(1, &View.Viewport);
    CommandList->RSSetScissorRects(1, &View.ScissorRect);
    CommandList->OMSetRenderTargets(1, &View.RTV, FALSE, nullptr);

    constexpr float ClearColor[4] = {0.05f, 0.08f, 1.00f, 1.f};

    CommandList->ClearRenderTargetView(View.RTV, ClearColor, 0, nullptr);

    Update_LightingConstants();
    
    Result = m_pRenderManager.Render();
    if (FAILED(Result)) return Result;
    
    return m_pGraphicDevice.End_Frame();
}

HRESULT  GameInstance::ShutDown() {
    if (!m_pGraphicDevice.Get_Device()) return S_OK;
    m_pGraphicDevice.Discard_FrameForShutdown();
    
    HRESULT Result = m_pGraphicDevice.Wait_ForGPU();
    if (FAILED(Result)) return Result;
    
    Shutdown_TestMesh();
    
    m_bResizePending = false;
    
    m_pLightManager  .Shutdown();
    m_pSoundManager  .Shutdown();
    m_pGameController.Shutdown();
    m_pRenderManager .ShutDown();
    Result = m_pGraphicDevice.Shutdown();
    
    return Result;
}

GameInstance& GameInstance::Get_Instance() {
    static GameInstance instance;
    return instance;
}

HRESULT GameInstance::Initialize_TestMesh()
{
    if (m_TestMesh)
        return S_OK;

    auto* CommandList = m_pGraphicDevice.Get_CommandList();
    if (!CommandList)
        return E_UNEXPECTED;

    const Vertex Vertices[] = {
        {
            {-0.5f, -0.5f, 0.5f},
            { 0.f,   0.f, -1.f},
            { 0.f,   1.f}
        },
        {
                { 0.f,   0.5f, 0.5f},
                { 0.f,   0.f, -1.f},
                { 0.5f,  0.f}
        },
        {
                { 0.5f, -0.5f, 0.5f},
                { 0.f,   0.f, -1.f},
                { 1.f,   1.f}
        }
    };

    const uint32_t Indices[] = {0, 1, 2};

    auto Mesh = std::make_unique<MeshResource>();

    const HRESULT Result = Mesh->Initialize(
        m_pGraphicDevice.Get_Device(),
        CommandList,
        Vertices, static_cast<uint32_t>(_countof(Vertices)),
        Indices, static_cast<uint32_t>(_countof(Indices)));

    if (FAILED(Result))
        return Result;

    m_TestMesh = std::move(Mesh);
    
    return S_OK;
}
void GameInstance::Shutdown_TestMesh() {
    m_pRenderManager.Clear_DrawList();
    m_TestMesh.reset();
}

HRESULT GameInstance::Update_CameraProjection() {
    const auto& Desc = m_pGraphicDevice.Get_GraphicDeviceDesc();

    if (Desc.m_iWindowWidth == 0 || Desc.m_iWindowHeight == 0)  return E_INVALIDARG; // Window Scale must be over 1x1

    const float AspectRatio = static_cast<float>(Desc.m_iWindowWidth) / static_cast<float>(Desc.m_iWindowHeight);

    return m_Camera.Set_Perspective(DirectX::XM_PIDIV4, AspectRatio, 0.1f, 1000.f);
}

HRESULT GameInstance::Submit_TestMeshes() {
    if (!m_TestMesh)
        return E_UNEXPECTED;

    for (const float OffsetX : {-0.6f, 0.6f}) {
        const auto World =
            DirectX::XMMatrixScaling(0.7f, 0.7f, 0.7f) *
            DirectX::XMMatrixTranslation(OffsetX, 0.f, 0.f);

        TransformConstants Transform{};

        DirectX::XMStoreFloat4x4(&Transform.World, World);

        Transform.ViewProjection =
            m_Camera.Get_ViewProjection();

        DirectX::XMStoreFloat4x4(
            &Transform.WorldInverseTranspose,
            DirectX::XMMatrixTranspose(
                DirectX::XMMatrixInverse(nullptr, World)));

        const HRESULT Result = m_pRenderManager.Submit_Mesh(*m_TestMesh, Transform);

        if (FAILED(Result))
            return Result;
    }

    return S_OK;
}

void GameInstance::Update_LightingConstants() {
    LightingConstants Constants{};

    const auto& ViewProjection = m_Camera.Get_ViewProjection();

    DirectX::XMStoreFloat4x4(&Constants.InverseViewProjection,
        DirectX::XMMatrixInverse(nullptr,DirectX::XMLoadFloat4x4(&ViewProjection)));

    const auto& Position = m_Camera.Get_Position();

    Constants.CameraPosition = { Position.x, Position.y, Position.z, 0.f };

    m_pRenderManager.Set_LightingConstants(Constants);
    m_pRenderManager.Set_DebugDepthRange(m_Camera.Get_NearZ(), m_Camera.Get_FarZ());
}
