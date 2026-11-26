#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"

ENGINE_START
class GameInstance;
class ENGINE_DLL EngineObjectPool final : public Interface::EngineEditor, Interface::Uncopyable {
    friend class GameInstance;
private:
    explicit EngineObjectPool()  = default;
    ~EngineObjectPool() override = default;
    
public:
    
};
ENGINE_END