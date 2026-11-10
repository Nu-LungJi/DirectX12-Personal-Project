#pragma once
#include "EngineCommonHeader/Engine_Default.h"
#include "EngineCommonHeader/Engine_Interface.h"

#include "GraphicDevice.h"
#include "TimeManager.h"
#include "GameController.h"
#include "RenderManager.h"
#include "SoundManager.h"
#include "LightManager.h"
#include "EngineUtility.h"


ENGINE_START
class ENGINE_DLL GameInstance final : public Interface::Uncopyable {
private:
    GameInstance()  = default;
    ~GameInstance() override = default;
	
public:
    HRESULT Initialize();
    void Update();
    void LateUpdate();
    void Render();
	
    [[nodiscard("Warning : Store Instance after Create.")]]
    static GameInstance&	Get_Instance();
	
    [[nodiscard("Warning : Use or Store Instance.")]]
    const GraphicDevice&	Get_GraphicDevice() const noexcept { return m_pGraphicDevice; }
	
    [[nodiscard("Warning : Use or Store Instance.")]]
    const TimeManager&		Get_TimeManager()	const noexcept { return m_pTimeManager; }
	
    [[nodiscard("Warning : Use or Store Instance.")]]
    const RenderManager&	Get_RenderManager() const noexcept { return m_pRenderManager; }
	
    [[nodiscard("Warning : Use or Store Instance.")]]
    const GameController&	Get_GameController() const noexcept { return m_pGameController; }
	
    [[nodiscard("Warning : Use or Store Instance.")]]
    const EngineUtility&	Get_EngineUtility() const noexcept { return m_pEngineUtility; }
	
public:
    void SetInputFocused(bool focused) noexcept { m_pGameController.SetInputFocused(focused); }
    void RequestTimeReset() noexcept { m_pTimeResetRequested.store(true,std::memory_order_release); }

private:
    GraphicDevice	m_pGraphicDevice;
    TimeManager		m_pTimeManager;
    GameController	m_pGameController;
    RenderManager	m_pRenderManager;
    SoundManager	m_pSoundManager;
    LightManager	m_pLightManager;
	
    EngineUtility	m_pEngineUtility;
	
private:
    std::atomic<bool> m_pTimeResetRequested{false};
};
ENGINE_END