#include <FMOD/fmod.hpp>
#include "SoundManager.h"
#include "SoundResource.h"
ENGINE_USE

SoundManager::SoundManager() = default;
SoundManager::~SoundManager() { Shutdown(); }

HRESULT SoundManager::Initialize(uint8_t MaxChannelCount) {
    if (m_pFMODSystem) return S_OK;
    if (MaxChannelCount <= 0) return E_FAIL;

    FMOD::System* FMODSystem = nullptr;

    FMOD_RESULT Result = FMOD::System_Create(&FMODSystem);
    if (Result != FMOD_OK) return E_FAIL;

    Result = FMODSystem->init(MaxChannelCount, FMOD_INIT_NORMAL, nullptr);

    if (Result != FMOD_OK) {
        FMODSystem->release();
        return E_FAIL;
    }

    m_pFMODSystem = FMODSystem;
    
    Result = m_pFMODSystem->createChannelGroup("BGM", &m_pBGMGroup);
    if (Result != FMOD_OK) { Shutdown(); return E_FAIL; }
    
    Result = m_pFMODSystem->createChannelGroup("SFX", &m_pSFXGroup);
    if (Result != FMOD_OK) { Shutdown(); return E_FAIL; }
    
    return S_OK;
}

void SoundManager::Update() {
    if (m_pFMODSystem) m_pFMODSystem->update();
}

void SoundManager::Shutdown() {
    if (m_pBGMGroup) m_pBGMGroup->stop();
    if (m_pSFXGroup) m_pSFXGroup->stop();
    
    m_SoundList.clear();

    if (m_pBGMGroup) {
        m_pBGMGroup->release();
        m_pBGMGroup = nullptr;
    }
    
    if (m_pSFXGroup) {
        m_pSFXGroup->release();
        m_pSFXGroup = nullptr;
    }
    
    if (m_pFMODSystem) {
        m_pFMODSystem->release();
        m_pFMODSystem = nullptr;
    }
}

bool SoundManager::Load_SoundAsset(const std::string& Key, const std::filesystem::path& Path, bool Spatial, bool Stream) {
    if (!m_pFMODSystem || Key.empty() || m_SoundList.contains(Key)) return false;
    
    FMOD_MODE mode = Spatial ? FMOD_3D : FMOD_2D;
    mode |= Stream ? FMOD_CREATESTREAM : FMOD_CREATESAMPLE;
    mode |= FMOD_LOOP_OFF;
    
    std::unique_ptr<SoundResource> Resource = std::unique_ptr<SoundResource>(new SoundResource(nullptr));
    
    const auto utf8Path = Path.u8string();
    const std::string filePath(utf8Path.begin(), utf8Path.end());
    const FMOD_RESULT Result = m_pFMODSystem->createSound(filePath.c_str(), mode, nullptr, &Resource->m_pSound);
    
    if (Result != FMOD_OK) return false;
    
    m_SoundList.emplace(Key, std::move(Resource));
    
    return true;
}

FMOD::Channel* SoundManager::Play_Sound(const std::string& Key, float VolumeValue, bool LoopState, SoundGroup Group) {
    if (!m_pFMODSystem) return nullptr;
    
    const auto Res = m_SoundList.find(Key);
    if (Res == m_SoundList.end() || !Res->second->m_pSound) return nullptr;
    
    FMOD::Channel* Channel = nullptr;
    
    const FMOD_RESULT FResult = m_pFMODSystem->playSound(Res->second->m_pSound, 
        Group == SoundGroup::BGM ? m_pBGMGroup : m_pSFXGroup, true, &Channel);
    if (FResult != FMOD_OK) return nullptr;
    
    if (FAILED(Set_SoundVolume(Channel, VolumeValue)) || FAILED(Set_SoundLoop(Channel, LoopState)) 
        || FAILED(Set_SoundPaused(Channel, false))) {
        Channel->stop();
        return nullptr;
    }
    
    return Channel;
}

HRESULT SoundManager::Stop_Sound(FMOD::Channel* Channel) {
    return Channel && Channel->stop() == FMOD_OK ? S_OK : E_FAIL;
}

HRESULT SoundManager::Set_SoundVolume(FMOD::Channel* Channel, float Volume) {
    return Channel && std::isfinite(Volume) && Volume >= 0.f
        && Channel->setVolume(Volume) == FMOD_OK ? S_OK : E_FAIL;
}

HRESULT SoundManager::Set_SoundPaused(FMOD::Channel* Channel, bool State) {
    return Channel && Channel->setPaused(State) == FMOD_OK ? S_OK : E_FAIL;
}

HRESULT SoundManager::Set_SoundLoop(FMOD::Channel* Channel, bool State) {
    if (!Channel) return E_FAIL;
    
    const FMOD_RESULT FResult = Channel->setMode(State ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
    if (FResult != FMOD_OK) return E_FAIL;
    
    constexpr int8_t LOOP_INFINITY = -1;
    
    return Channel->setLoopCount(State ? LOOP_INFINITY : 0) == FMOD_OK ? S_OK : E_FAIL;;
}

HRESULT SoundManager::Set_GroupVolume(SoundGroup Group, float Volume) {
    FMOD::ChannelGroup* TargetGroup = Group == SoundGroup::BGM ? m_pBGMGroup : m_pSFXGroup;

    return TargetGroup && std::isfinite(Volume) && Volume >= 0.f && 
        TargetGroup->setVolume(Volume) == FMOD_OK ? S_OK : E_FAIL;
}
