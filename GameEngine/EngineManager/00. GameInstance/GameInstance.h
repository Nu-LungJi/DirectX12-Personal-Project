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
#include "Camera.h"

ENGINE_START
class ENGINE_DLL GameInstance final : public Interface::Uncopyable {
private:
    GameInstance()  = default;
    ~GameInstance() override;
	
public:
	[[nodiscard("Warning : Check GameInstance Initialize Result.")]]
    HRESULT Initialize(const GraphicDeviceDesc& GraphicDesc);
    void	Update();
    void	LateUpdate();
	
	[[nodiscard("Warning : Check Render Initialize Result.")]]
    HRESULT Render() noexcept;
	
	[[nodiscard("Warning : Check ShutDown Initialize Result.")]]
	HRESULT ShutDown();
	
public:
	[[nodiscard("Warning : GameInstance value is discarded. Store or use the value.")]]
    static GameInstance&	Get_Instance();
	
	[[nodiscard("Warning : GraphicDevice value is discarded. Store or use the value.")]]
    const GraphicDevice&	Get_GraphicDevice() const noexcept { return m_pGraphicDevice; }
	
	[[nodiscard("Warning : TimeManager value is discarded. Store or use the value.")]]
    const TimeManager&		Get_TimeManager()	 const noexcept { return m_pTimeManager; }
	
	[[nodiscard("Warning : GameController value is discarded. Store or use the value.")]]
	const GameController&	Get_GameController() const noexcept { return m_pGameController; }
	
	[[nodiscard("Warning : RenderManager value is discarded. Store or use the value.")]]
    const RenderManager&	Get_RenderManager()  const noexcept { return m_pRenderManager; }
	
	[[nodiscard("Warning : SoundManager value is discarded. Store or use the value.")]]
	const SoundManager&		Get_SoundManager()   const noexcept { return m_pSoundManager; }
	
	[[nodiscard("Warning : ShaderManager value is discarded. Store or use the value.")]]
	const ShaderManager&	Get_ShaderManager()   const noexcept { return m_pShaderManager; }
	
	[[nodiscard("Warning : LightManager value is discarded. Store or use the value.")]]
	const LightManager&		Get_LightManager()    const noexcept { return m_pLightManager; }
	
public:
	[[nodiscard("Warning : EngineUtility value is discarded. Store or use the value.")]]
    const EngineUtility&	Get_EngineUtility()   const noexcept { return m_pEngineUtility; }
	

public:
    void Set_InputFocused(bool focused) noexcept { m_pGameController.Set_InputFocused(focused); }
    void Request_TimeReset() noexcept { m_pTimeResetRequested.store(true,std::memory_order_release); }

	void Request_Resize(uint32_t Width, uint32_t Height) noexcept
    {
    	m_iPendingWidth = Width;
    	m_iPendingHeight = Height;
    	m_bResizePending = true;
    }
	
	HRESULT Initialize_TestMesh();
	void Shutdown_TestMesh();
	
	HRESULT Update_CameraProjection();
	
public:
	void Dump_DebugMessages() const { m_pGraphicDevice.Dump_DebugMessages(); }
	
private:
	HRESULT Submit_TestMeshes();
	
	void Update_LightingConstants();
	
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
	
	uint32_t m_iPendingWidth  = 0;
	uint32_t m_iPendingHeight = 0;
	bool	 m_bResizePending = false;

private:
	std::unique_ptr<MeshResource> m_TestMesh;
	Camera m_Camera;
};
ENGINE_END