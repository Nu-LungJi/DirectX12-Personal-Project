#pragma once

#include <filesystem>
#include <unordered_map>
#include "Engine_Default.h"
#include "Engine_Interface.h"

enum class SoundGroup { BGM, SFX };

namespace FMOD {
    class System;
    class ChannelGroup;
    class Channel;
}

ENGINE_START
struct SpatialSoundDesc {
    DirectX::XMFLOAT3 m_Position{};
	
    float m_fMinDistance{};
    float m_fMaxDistance{};
};

class GameInstance;

class ENGINE_DLL SoundManager final : public Interface::Uncopyable {
    friend class GameInstance;
private:
    SoundManager();
    ~SoundManager() override;
    
public:
    [[nodiscard("Warning : Check SoundManager Initialize Result.")]]
    HRESULT Initialize(uint8_t MaxChannelCount = 128);
	
    void Update();
    
    void Shutdown();
    
    [[nodiscard("Warning : Check SoundManager Loaded SoundAsset Properly.")]]
    bool Load_SoundAsset(const std::string& Key, const std::filesystem::path& Path, bool Spatial = false, bool Stream = false);
    
public:
    [[nodiscard("Warning : Returned FMOD::Channel* is required for controlling the playing sound.")]]
    FMOD::Channel* Play_Sound(const std::string& Key, float VolumeValue = 1.f, bool LoopState = false, SoundGroup Group = SoundGroup::SFX);
    
    [[nodiscard("Warning : Check to verify if the channel stopped successfully.")]]
    HRESULT Stop_Sound(FMOD::Channel* Channel);
    
    [[nodiscard("Warning : Check to verify if 'Volume' adjustment succeeded.")]]
    HRESULT Set_SoundVolume(FMOD::Channel* Channel, float Volume);
    
    [[nodiscard("Warning : Check to verify if 'Paused State' changed successfully.")]]
    HRESULT Set_SoundPaused(FMOD::Channel* Channel, bool State);
    
    [[nodiscard("Warning : Check to verify if 'Loop State' was applied successfully.")]]
    HRESULT Set_SoundLoop(FMOD::Channel* Channel, bool State);
    
public:
    [[nodiscard("Warning : Check to verify if 'Group Volume' adjustment succeeded.")]]
    HRESULT Set_GroupVolume(SoundGroup Group, float Volume);
    
private:
    FMOD::System* m_pFMODSystem = nullptr;
    
    FMOD::ChannelGroup* m_pBGMGroup = nullptr;
    FMOD::ChannelGroup* m_pSFXGroup = nullptr;
    
    std::unordered_map<std::string, std::unique_ptr<class SoundResource>> m_SoundList;
};

ENGINE_END