#include "EngineDebugger.h"
#include "EngineLogger.h"
#include <iomanip>
#include <sstream>

#include "EngineUtility.h"
ENGINE_USE

namespace {
    std::wstring Get_HRESULTDescription(HRESULT Result) {
        wchar_t Buffer[512]{};

        const DWORD Length = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, static_cast<DWORD>(Result), 0, Buffer,
            512, nullptr);

        if (Length == 0) return L"No system description available.";

        std::wstring Message(Buffer, Length);

        while (!Message.empty() && (Message.back() == L'\r' || Message.back() == L'\n'))
            Message.pop_back();

        return Message;
    }
    
}

HRESULT EngineDebugger::Check_Result(EngineLogger& Logger, HRESULT Result, std::string_view Expression,
                                     std::source_location Location) {
    if (SUCCEEDED(Result)) return Result;
    
    std::wostringstream Stream;
    
    // Error Summary
    Stream << L"\n[DX12][ERROR] ";
    const std::wstring Description = Get_HRESULTDescription(Result);

    if (Description  != L"UNKNOWN_HRESULT")
        Stream << Description  << L' ';

    Stream
        << L"(0x" << std::hex << std::uppercase << std::setw(8) << std::setfill(L'0')
        << static_cast<std::uint32_t>(Result) << L")\n" << std::dec;
    
    // Error Details
    Stream
        << L"  Location : " << EngineUtility::Convert_WideString(Location.file_name()) << L"(" << Location.line() << L")\n"
        << L"  Function : " << EngineUtility::Convert_WideString(Location.function_name()) << L'\n'
        << L"  Message  : " << Get_HRESULTDescription(Result) << L'\n'
        << L"  Call     : " << EngineUtility::Convert_WideString(Expression) << L'\n';

    (void)EngineLogger::Error(LogChannels::Graphics, Stream.str(), Location);
    
    return Result;
}

HRESULT EngineDebugger::Enable_DebugLayer(bool GPUValidation) {
#ifdef _DEBUG
    ComPtr<ID3D12Debug> DBG_Controller;

    HRESULT Result = D3D12GetDebugInterface(IID_PPV_ARGS(&DBG_Controller));
    if (FAILED(Result)) return Result;

    DBG_Controller->EnableDebugLayer();

    if (GPUValidation) {
        ComPtr<ID3D12Debug1> DBG_Controller1;

        Result = DBG_Controller.As(&DBG_Controller1);
        if (FAILED(Result)) return Result;

        DBG_Controller1->SetEnableGPUBasedValidation(TRUE);
    }

    return S_OK;

#else
    (void)GPUValidation;
    return S_FALSE;
#endif
}

void EngineDebugger::Assert_Failure(EngineLogger& Logger, std::string_view Expression, std::source_location Location) {
    std::wostringstream Stream;

    // Assertion Summary
    Stream << L"\n[ENGINE][ASSERT] Assertion Failed\n";
    
    // Assertion Details
    Stream
        << L"  Condition : " << EngineUtility::Convert_WideString(Expression) << L'\n'
        << L"  Location  : " << EngineUtility::Convert_WideString(Location.file_name())
        << L"(" << Location.line() << L")\n"
        << L"  Function  : " << EngineUtility::Convert_WideString(Location.function_name()) << L'\n';

    (void)EngineLogger::Critical(LogChannels::Diagnostics, Stream.str(), Location);
    
    if (IsDebuggerPresent()) __debugbreak();

    std::abort();
}

HRESULT EngineDebugger::Set_ObjectName(ID3D12Object* Object, const wchar_t* Name) {
    return (!Object || !Name) ? E_POINTER : Object->SetName(Name);
}
