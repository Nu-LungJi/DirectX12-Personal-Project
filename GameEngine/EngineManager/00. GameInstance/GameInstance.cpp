#include "GameInstance.h"
#include "GraphicDevice.h"
#include "TimeManager.h"
#include "SoundManager.h"
#include "GameController.h"
ENGINE_USE

HRESULT GameInstance::Initialize(const GraphicDeviceDesc& GraphicDesc) {
    HRESULT Result = m_pGraphicDevice.Initialize(GraphicDesc);
    if (FAILED(Result)) return Result;
    
    Result = m_pGameController.Initialize();
    if (FAILED(Result)) return Result;
        
    Result = m_pShaderManager.Initialize();
    if (FAILED(Result)) return Result;
    
    Result = m_pSoundManager.Initialize();
    if (FAILED(Result)) return Result;
    
    
    
    

    
    m_pTimeManager.Reset();
    
    return S_OK;
}

void GameInstance::Update() {
    const bool ResetTime = m_pTimeResetRequested.exchange(false, std::memory_order_acq_rel);
    ResetTime ? m_pTimeManager.Reset() : m_pTimeManager.Update();

    m_pGameController.BeginFrame();
    m_pShaderManager.Update();
    m_pSoundManager.Update();
}

void GameInstance::LateUpdate() {
    
}

HRESULT GameInstance::Render() noexcept {
    if (m_bResizePending)
    {
        m_bResizePending = false;

        const HRESULT Result = m_pGraphicDevice.Resize(
            m_iPendingWidth, m_iPendingHeight);

        if (FAILED(Result))
            return Result;
    }

    HRESULT Result = m_pGraphicDevice.Begin_Frame();

    if (Result != S_OK)
        return Result;

    auto* CommandList = m_pGraphicDevice.Get_CommandList();
    const auto View = m_pGraphicDevice.Get_CurrentView();

    CommandList->RSSetViewports(1, &View.Viewport);
    CommandList->RSSetScissorRects(1, &View.ScissorRect);
    CommandList->OMSetRenderTargets(1, &View.RTV, FALSE, nullptr);

    const float ClearColor[4] = {0.05f, 0.08f, 0.12f, 1.f};

    CommandList->ClearRenderTargetView(
        View.RTV, ClearColor, 0, nullptr);

    return m_pGraphicDevice.End_Frame();
}

GameInstance& GameInstance::Get_Instance() {
    static GameInstance instance;
    return instance;
}