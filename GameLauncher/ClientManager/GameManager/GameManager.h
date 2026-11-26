#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"
#include "GameInstance.h"

class GameManager final : public Interface::Uncopyable {
private:
    explicit GameManager();
    ~GameManager();
    
public:
    HRESULT Initialize(const ENGINE::GraphicDeviceDesc& GraphicDesc) const noexcept;
    void Update() const noexcept;
    void LateUpdate() const noexcept;
    HRESULT Render() const noexcept;
    
    HRESULT ShutDown() const noexcept;
    
    [[nodiscard("Warning : Store Instance after Create.")]]
    static GameManager& Create();
    
private:
    ENGINE::GameInstance& m_pGameInstance;
};

