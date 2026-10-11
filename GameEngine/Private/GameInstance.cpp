#include "GameInstance.h"
#include "GraphicDevice.h"
#include "TimeManager.h"

ENGINE_USE

HRESULT GameInstance::Initialize() {
    const HRESULT HR = m_pGameController.Initialize();
    if (FAILED(HR)) return HR;
        
    
    m_pTimeManager.Reset();
    return S_OK;
}

void GameInstance::Update() {
    const bool ResetTime = m_pTimeResetRequested.exchange(false, std::memory_order_acq_rel);
    ResetTime ? m_pTimeManager.Reset() : m_pTimeManager.Update();

    m_pGameController.BeginFrame();
}

void GameInstance::LateUpdate() {
    
}

void GameInstance::Render() {
    
}

GameInstance& GameInstance::Get_Instance() {
    static GameInstance instance;
    return instance;
}
