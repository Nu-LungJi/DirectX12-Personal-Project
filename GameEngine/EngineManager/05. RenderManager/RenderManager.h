#pragma once
#include "EngineCommonHeader/Engine_Default.h"
#include "EngineCommonHeader/Engine_Interface.h"

ENGINE_START

class GameInstance;

class ENGINE_DLL RenderManager : public Interface::Uncopyable {
    friend class GameInstance;
private:
    explicit RenderManager() = default;
    ~RenderManager() override;
	
public:
    [[nodiscard("Warning : Check SoundManager Initialize Result.")]]
    bool Initialize(uint8_t MaxChannel = 128);
	
    void Update();
    
    void Shutdown();

private:
    
};
ENGINE_END