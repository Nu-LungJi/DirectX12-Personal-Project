#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"

ENGINE_START
class GameInstance;
class ENGINE_DLL Utility : public Interface::Uncopyable {
    friend class GameInstance;
private:
    explicit Utility() = default;
    ~Utility() override = default;

public:
    static HRESULT ErrorMassage(HRESULT hr, std::wstring_view location);
};
ENGINE_END
