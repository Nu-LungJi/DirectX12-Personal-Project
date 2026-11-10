#pragma once

#include "EngineCommonHeader/Engine_Default.h"
#include "EngineCommonHeader/Engine_Interface.h"
#include <filesystem>

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
    explicit SoundManager() = default;
    ~SoundManager() override = default;
    
public:
    [[nodiscard("Warning : Check SoundManager Initialize Result.")]]
    bool Initialize(uint8_t MaxChannel = 128);
	
    void Shutdown();
    
    [[nodiscard("Warning : Check SoundManager Loaded SoundAsset Properly.")]]
    bool Load_SoundAsset(const std::string& key, const std::filesystem::path& path, bool spatial = false);
    
private:
    FMOD::System* m_FMODSystem{};
};

ENGINE_END