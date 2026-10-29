#include "PCH.h"
#include "GameManager.h"
#include "GameInstance.h"

GameManager::GameManager()  : m_pGameInstance{ENGINE::GameInstance::Get_Instance()} {}
GameManager::~GameManager() = default;

HRESULT GameManager::Initialize() {
    return m_pGameInstance.Initialize();
}

void GameManager::Update() noexcept {
    m_pGameInstance.Update();
}

void GameManager::LateUpdate() noexcept {
    
}

void GameManager::Render() noexcept {
    
}

GameManager& GameManager::Create() {
    static GameManager instance;
    return instance;
}
