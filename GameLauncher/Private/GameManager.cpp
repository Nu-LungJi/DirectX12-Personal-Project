#include "PCH.h"
#include "GameManager.h"
#include "GameInstance.h"

GameManager::GameManager()  : m_pGameInstance{ENGINE::GameInstance::Get_Instance()} {}
GameManager::~GameManager() = default;

HRESULT GameManager::Initialize(const ENGINE::GraphicDeviceDesc& GraphicDesc) {
    return m_pGameInstance.Initialize(GraphicDesc);
}

void GameManager::Update() noexcept {
    m_pGameInstance.Update();
}

void GameManager::LateUpdate() noexcept {
    m_pGameInstance.LateUpdate();   
}

HRESULT GameManager::Render() noexcept {
    return m_pGameInstance.Render();
}

GameManager& GameManager::Create() {
    static GameManager instance;
    return instance;
}
