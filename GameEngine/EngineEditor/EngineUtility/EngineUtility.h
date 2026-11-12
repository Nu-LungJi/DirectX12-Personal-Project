#pragma once
#include "EngineCommonHeader/Engine_Default.h"
#include "EngineCommonHeader/Engine_Interface.h"

ENGINE_START
class GameInstance;
class ENGINE_DLL EngineUtility : public Interface::Uncopyable {
    friend class GameInstance;
private:
    explicit EngineUtility() = default;
    ~EngineUtility() override = default;

public:
    static HRESULT ErrorMassage(HRESULT hr, std::wstring_view location);
};
ENGINE_END
