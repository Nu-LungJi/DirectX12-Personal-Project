#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"

ENGINE_START
class ENGINE_DLL DebugLogSink final : public Interface::Uncopyable {
public:
    [[nodiscard("Check DebugLog wrote messages properly.")]]
    bool Write(std::string_view Message) const;
};
ENGINE_END