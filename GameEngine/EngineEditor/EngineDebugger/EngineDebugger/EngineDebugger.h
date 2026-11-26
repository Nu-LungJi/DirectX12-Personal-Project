#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"

ENGINE_START

// EngineDebugger Role : Provides common DirectX 12 debugging utilities for error detection and diagnostics.
//                       Tracks API failures, validates conditions, and improves debugging efficiency.

// std::source_location : provide location INFO(such as file name, line numb, function name) 
//                        for the current source code as a single object.
// usually use 'current()' with Default Argument. it captures the location from which the function was called.

class EngineLogger;

class ENGINE_DLL EngineDebugger final : public Interface::EngineEditor {
public:
    EngineDebugger() = delete;
    
public:
    // Checks HRESULT failures and logs error details and source location.
    [[nodiscard("Warning : Check EngineDebugger Check_Result Result.")]]
    static HRESULT  Check_Result(EngineLogger& Logger, HRESULT Result, std::string_view Expression,
        std::source_location Location = std::source_location::current());
    
    // Enables the D3D12 Debug Layer to detect API usage errors.
    [[nodiscard("Warning : Check EngineDebugger Enable_DebugLayer Result.")]]
    static HRESULT  Enable_DebugLayer(bool GPUValidation = false);
    
    // Logs assertion failures and breaks into the debugger.
    [[noreturn]]
    static void     Assert_Failure(EngineLogger& Logger, std::string_view Expression,
        std::source_location Location = std::source_location::current());
    
    // Assigns a debug name to a DirectX 12 object for easier identification.
    [[nodiscard]]
    static HRESULT  Set_ObjectName(ID3D12Object* Object, const wchar_t* Name);
};
ENGINE_END