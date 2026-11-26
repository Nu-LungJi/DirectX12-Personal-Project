#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"

ENGINE_START
class GameInstance;
class ENGINE_DLL EngineThreadPool final : public Interface::EngineEditor, Interface::Uncopyable {
    friend class GameInstance;
private:
    explicit EngineThreadPool()  = default;
    ~EngineThreadPool() override = default;
    
public:
    
};
ENGINE_END