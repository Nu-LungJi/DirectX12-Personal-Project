#include "TimeManager.h"
ENGINE_USE

void TimeManager::Update() {
    std::unique_lock lock{m_pMutex};

    const TimePoint CurrentFrameTime = Clock::now();

    m_pUnscaledDeltaTime = Seconds{CurrentFrameTime - m_pLastFrameTime};
    m_pLastFrameTime     = CurrentFrameTime;

    m_pDeltaTime = m_pGamePauseState ? m_pZeroTime : m_pUnscaledDeltaTime * m_pTimeScale;
}

void TimeManager::Reset() {
    std::unique_lock lock{m_pMutex};
    
    m_pLastFrameTime = Clock::now();
    m_pUnscaledDeltaTime = m_pDeltaTime = m_pZeroTime;
}