#include "GameManager.h"
#include "GameInstance.h"

GameManager::GameManager()  : m_pGameInstance{ENGINE::GameInstance::Get_Instance()} {}
GameManager::~GameManager() = default;

HRESULT GameManager::Initialize(const ENGINE::GraphicDeviceDesc& GraphicDesc) const noexcept {
    return m_pGameInstance.Initialize(GraphicDesc);
}

void GameManager::Update() const noexcept {
    m_pGameInstance.Update();
}

void GameManager::LateUpdate() const noexcept {
    m_pGameInstance.LateUpdate();   
}

HRESULT GameManager::Render() const noexcept {
    return m_pGameInstance.Render();
}

HRESULT GameManager::ShutDown() const noexcept {
    return m_pGameInstance.ShutDown();
}

GameManager& GameManager::Create() {
    static GameManager instance;
    return instance;
}