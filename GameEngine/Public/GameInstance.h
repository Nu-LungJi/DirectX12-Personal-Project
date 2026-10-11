#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"

#include "GraphicDevice.h"
#include "TimeManager.h"
#include "RenderManager.h"
#include "GameController.h"
#include "Utility.h"

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
	const Utility&			Get_Utility() const noexcept { return m_pUtility; }
	
public:
	void SetInputFocused(bool focused) noexcept { m_pGameController.SetInputFocused(focused); }
	void RequestTimeReset() noexcept { m_pTimeResetRequested.store(true,std::memory_order_release); }

private:
	GraphicDevice	m_pGraphicDevice;
	TimeManager		m_pTimeManager;
	RenderManager	m_pRenderManager;
	GameController	m_pGameController;
	Utility			m_pUtility;
	
private:
	std::atomic<bool> m_pTimeResetRequested{false};
};
ENGINE_END