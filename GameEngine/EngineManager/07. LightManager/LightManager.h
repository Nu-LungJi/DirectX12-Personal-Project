#pragma once

#include "EngineCommonHeader/Engine_Default.h"
#include "EngineCommonHeader/Engine_Interface.h"

ENGINE_START

class GameInstance;

class ENGINE_DLL LightManager final : public Interface::Uncopyable {
    friend class GameInstance;
private:
    explicit LightManager() = default;
    ~LightManager() override = default;
    
};

ENGINE_END