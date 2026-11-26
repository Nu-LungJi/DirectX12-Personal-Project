#include "EngineUtility.h"
ENGINE_USE
HRESULT EngineUtility::ErrorMassage(HRESULT hr, std::wstring_view location) {
    if (SUCCEEDED(hr)) return hr;

    wchar_t description[1024]{};

    const DWORD messageId = HRESULT_FACILITY(hr) == FACILITY_WIN32 ? static_cast<DWORD>(HRESULT_CODE(hr)) : static_cast<DWORD>(hr);
    const DWORD length = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, messageId, 0, description, static_cast<DWORD>(sizeof(description) / sizeof(wchar_t)), nullptr);

    const std::wstring_view detail = length != 0 ? std::wstring_view{description, length} : std::wstring_view{L"No system description is available."};

    const auto message = std::format(L"Location: {}\n" L"HRESULT: 0x{:08X}\n\n{}", location, static_cast<std::uint32_t>(hr), detail);

    MessageBoxW(nullptr, message.c_str(), L"Initialization Error", MB_OK | MB_ICONERROR);

    return hr;
}

XMFLOAT2 EngineUtility::Convert_WorldPositionToScreen(XMVECTOR _WorldPosition) {
    XMVECTOR CurrentPos = XMVectorSetW(_WorldPosition, 1.f);
    return XMFLOAT2{0.f, 0.f};
}

// Converts a UTF-8 string to UTF-16.
std::wstring EngineUtility::Convert_WideString(std::string_view str) {
    if (str.empty()) return {};
    
    if (str.size() > INT_MAX) return L"(Text too long)";
        
    const int Length = MultiByteToWideChar(CP_UTF8, 0, str.data(), 
        static_cast<int>(str.size()), nullptr, 0 );
        
    if (Length <= 0) return L"(Conversion Failed)";
        
    std::wstring Result(Length, L'\0');
    
    if (MultiByteToWideChar(CP_UTF8, 0, str.data(), 
        static_cast<int>(str.size()), Result.data(), Length) == 0) {
        return L"(Conversion failed)";
    }
    
    MultiByteToWideChar(CP_UTF8, 0, str.data(), 
        static_cast<int>(str.size()), Result.data(), Length);

    return Result;
}

std::string EngineUtility::Convert_NormalString(std::wstring_view str) {
    if (str.empty()) return {};

    // Win32 APIs accept an int for the input length.
    if (str.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)())) return {};

    const int InputSize = static_cast<int>(str.size());

    const int RequiredSize = WideCharToMultiByte(
        CP_UTF8,
        0,
        str.data(),
        InputSize,
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (RequiredSize == 0) return {};

    std::string Result(static_cast<std::size_t>(RequiredSize), '\0');

    const int ConvertedSize = WideCharToMultiByte(
        CP_UTF8,
        0,
        str.data(),
        InputSize,
        Result.data(),
        RequiredSize,
        nullptr,
        nullptr
    );

    if (ConvertedSize == 0) return {};

    return Result;
}

// Returns the symbolic name of an HRESULT.
std::wstring_view EngineUtility::Get_HRESULTName(HRESULT Result) {
    switch (Result) {
    case E_FAIL:                    return L"E_FAIL";
    case E_INVALIDARG:              return L"E_INVALIDARG";
    case E_POINTER:                 return L"E_POINTER";
    case E_OUTOFMEMORY:             return L"E_OUTOFMEMORY";
    case E_ACCESSDENIED:            return L"E_ACCESSDENIED";
    case DXGI_ERROR_DEVICE_REMOVED: return L"DXGI_ERROR_DEVICE_REMOVED";
    case DXGI_ERROR_DEVICE_HUNG:    return L"DXGI_ERROR_DEVICE_HUNG";
    case DXGI_ERROR_DEVICE_RESET:   return L"DXGI_ERROR_DEVICE_RESET";
    case DXGI_ERROR_INVALID_CALL:   return L"DXGI_ERROR_INVALID_CALL";
    default:                        return L"UNKNOWN_HRESULT";
    }
}