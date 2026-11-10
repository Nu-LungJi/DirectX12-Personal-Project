#pragma once
#include "EngineCommonHeader/Engine_Default.h"
#include "EngineCommonHeader/Engine_Interface.h"

#include <algorithm>
#include <chrono>
#include <mutex>
#include <shared_mutex>

ENGINE_START

class GameInstance;

class ENGINE_DLL TimeManager final : public Interface::Uncopyable {
	friend class GameInstance;
private:
	explicit TimeManager() = default;
	~TimeManager() override = default;

public:
	using Clock		= std::chrono::steady_clock;			// Not Affected By System Setting
	using TimePoint = Clock::time_point;
	using Seconds   = std::chrono::duration<double>;
	
public:
	void	Update();
	void	Reset();

public:
	// Game DeltaTime : Seconds elapsed between frames, affected by TimeScale.
	[[nodiscard("Warning : Use DeltaTime Value.")]]
	double	Get_DeltaTime() const {
		std::shared_lock lock{m_pMutex};
		return m_pDeltaTime.count();
	}
	
	// Unscaled DeltaTime : Seconds elapsed between frames, Not affected by TimeScale.
	[[nodiscard("Warning : Use Get_UnscaledDeltaTime Value.")]]
	double	Get_UnscaledDeltaTime() const {
		std::shared_lock lock{m_pMutex};
		return m_pUnscaledDeltaTime.count();
	}
	
	// Time Scale : Multiplier applied to Game DeltaTime.
	[[nodiscard("Warning : Use TimeScale Value.")]]
	double	Get_TimeScale() const {
		std::shared_lock lock{m_pMutex};
		return m_pTimeScale;
	}
	
	// Changes the time scale applied during the next Update().
	void	Set_TimeScale(double Scale) {
		std::unique_lock lock{m_pMutex};
		m_pTimeScale = (std::max)(0.0, Scale);
	}
	
	// Game Pause State : Returns whether gameplay time is paused. - [True : Paused / False : Running]
	[[nodiscard("Warning : Use GamePauseState Value.")]]
	bool	Get_GamePauseState() const {
		std::shared_lock lock{m_pMutex};
		return m_pGamePauseState;
	}
	
	// Changes the pause state applied during the next Update().
	void	Set_GamePauseState(bool State) {
		std::unique_lock lock{m_pMutex};
		m_pGamePauseState = State;
	}

private:
	static constexpr Seconds	m_pZeroTime = Seconds::zero();
	static constexpr double		m_pDefaultTimeScale = 1.0;
	
	mutable std::shared_mutex	m_pMutex;
	
private:
	TimePoint 			m_pLastFrameTime{Clock::now()};
	
	Seconds				m_pDeltaTime{m_pZeroTime};
	Seconds				m_pUnscaledDeltaTime{m_pZeroTime};
	
	double				m_pTimeScale{m_pDefaultTimeScale};
	bool				m_pGamePauseState{false};
};
ENGINE_END
