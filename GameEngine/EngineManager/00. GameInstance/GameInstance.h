#pragma once
#include "EngineCommonHeader/Engine_Default.h"
#include "EngineCommonHeader/Engine_Interface.h"

#include "GraphicDevice.h"
#include "TimeManager.h"
#include "GameController.h"
#include "RenderManager.h"
#include "SoundManager.h"
#include "ShaderManager.h"
#include "LightManager.h"
#include "EngineUtility.h"


ENGINE_START
class ENGINE_DLL GameInstance final : public Interface::Uncopyable {
private:
    GameInstance()  = default;
    ~GameInstance() override = default;
	
public:
    HRESULT Initialize(const GraphicDeviceDesc& GraphicDesc);
    void Update();
    void LateUpdate();
    HRESULT Render() noexcept;
	
    [[nodiscard("Warning : Store Instance after Create.")]]
    static GameInstance&	Get_Instance();
	
    [[nodiscard("Warning : Use or Store Instance.")]]
    const GraphicDevice&	Get_GraphicDevice() const noexcept { return m_pGraphicDevice; }
	
    [[nodiscard("Warning : Use or Store Instance.")]]
    const TimeManager&		Get_TimeManager()	 const noexcept { return m_pTimeManager; }
	
	[[nodiscard("Warning : Use or Store Instance.")]]
	const GameController&	Get_GameController() const noexcept { return m_pGameController; }
	
    [[nodiscard("Warning : Use or Store Instance.")]]
    const RenderManager&	Get_RenderManager()  const noexcept { return m_pRenderManager; }
	
	[[nodiscard("Warning : Use or Store Instance.")]]
	const SoundManager&		Get_SoundManager()   const noexcept { return m_pSoundManager; }
	
	[[nodiscard("Warning : Use or Store Instance.")]]
	const ShaderManager&	Get_ShaderManager()   const noexcept { return m_pShaderManager; }
	
	[[nodiscard("Warning : Use or Store Instance.")]]
	const LightManager&		Get_LightManager()    const noexcept { return m_pLightManager; }
	
public:
    [[nodiscard("Warning : Use or Store Instance.")]]
    const EngineUtility&	Get_EngineUtility()   const noexcept { return m_pEngineUtility; }
	

public:
    void Set_InputFocused(bool focused) noexcept { m_pGameController.SetInputFocused(focused); }
    void Request_TimeReset() noexcept { m_pTimeResetRequested.store(true,std::memory_order_release); }

	void Request_Resize(uint32_t Width, uint32_t Height) noexcept
    {
    	m_iPendingWidth = Width;
    	m_iPendingHeight = Height;
    	m_bResizePending = true;
    }
private:
    GraphicDevice	m_pGraphicDevice;
    TimeManager		m_pTimeManager;
    GameController	m_pGameController;
    RenderManager	m_pRenderManager;
    SoundManager	m_pSoundManager;
    ShaderManager	m_pShaderManager;
    LightManager	m_pLightManager;
	
    EngineUtility	m_pEngineUtility;
	
private:
    std::atomic<bool> m_pTimeResetRequested{false};
	
	uint32_t m_iPendingWidth = 0;
	uint32_t m_iPendingHeight = 0;
	bool m_bResizePending = false;
};
ENGINE_END