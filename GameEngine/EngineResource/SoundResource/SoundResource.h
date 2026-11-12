#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"
#include "Engine_Struct.h"

namespace FMOD {
    class Sound;
}

ENGINE_START
class SoundManager;
class ENGINE_DLL SoundResource final : public Interface::EngineResource {
    friend class SoundManager;
private:
    explicit SoundResource(FMOD::Sound* sound) : m_pSound(sound) {}
    
public:
    SoundResource(const SoundResource&) = delete;
    SoundResource& operator=(const SoundResource&) = delete;
    
public:
    ~SoundResource() override;
    
public:
    static const TypeInfo& StaticType() noexcept {
        static const TypeInfo type{ "ENGINE::SoundResource", &Interface::EngineResource::StaticType() };
        return type;
    }

    const TypeInfo& GetType() const noexcept override { return StaticType(); }

private:
    FMOD::Sound* m_pSound = nullptr;
};
ENGINE_END