#include "Utility.h"

HRESULT ENGINE::Utility::ErrorMassage(HRESULT hr, std::wstring_view location) {
    if (SUCCEEDED(hr)) return hr;

    wchar_t description[1024]{};

    const DWORD messageId = HRESULT_FACILITY(hr) == FACILITY_WIN32 ? static_cast<DWORD>(HRESULT_CODE(hr)) : static_cast<DWORD>(hr);
    const DWORD length = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, messageId, 0, description, static_cast<DWORD>(sizeof(description) / sizeof(wchar_t)), nullptr);

    const std::wstring_view detail = length != 0 ? std::wstring_view{description, length} : std::wstring_view{L"No system description is available."};

    const auto message = std::format(L"Location: {}\n" L"HRESULT: 0x{:08X}\n\n{}", 
        location, static_cast<std::uint32_t>(hr), detail);

    MessageBoxW(nullptr, message.c_str(), L"Initialization Error", MB_OK | MB_ICONERROR);

    return hr;
}
