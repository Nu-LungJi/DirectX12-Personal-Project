#include "DebugLogSink.h"
ENGINE_USE

bool DebugLogSink::Write(std::string_view Message) const {
    if (Message.empty()) return true;
    
    if (Message.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)())) return false;

    const int InputSize = static_cast<int>(Message.size());
    
    const int RequiredSize = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        Message.data(),
        InputSize,
        nullptr,
        0
    );

    if (RequiredSize == 0)
        return false;

    std::wstring WideMessage(static_cast<std::size_t>(RequiredSize), L'\0');

    // Convert UTF-8 to UTF-16.
    const int ConvertedSize = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        Message.data(),
        InputSize,
        WideMessage.data(),
        RequiredSize
    );

    if (ConvertedSize != RequiredSize)
        return false;

    // Send the message to the attached debugger.
    OutputDebugStringW(WideMessage.c_str());

    return true;
}
