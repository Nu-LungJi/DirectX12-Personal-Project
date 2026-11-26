#pragma once
#include "Engine_Default.h"
#include "Engine_Interface.h"

#include "LogTypeLookUp.h"
#include "FileLogSink.h"

ENGINE_START

template<typename... T>
struct LogFormat final {
    // Validate the string format with argument types on compile time() and get code location
    std::format_string<T...> m_Text;
    std::source_location     m_Location;
    
    template<std::size_t N>
    consteval LogFormat(const char (&Format)[N], std::source_location Location = std::source_location::current())
        : m_Text(Format), m_Location(Location) {}
};



class ENGINE_DLL EngineLogger final : public Interface::EngineEditor {
    using codeplace = std::source_location;
public:
    static bool Initialize(const std::filesystem::path& RootDirectory = L"EngineSystemLog");
    
    static bool Shutdown();
    
public:
    static bool Register_Channel(LogChannel Channel, std::string_view Name, const std::filesystem::path& FilePath,
            LogFileMode Mode = LogFileMode::Append, LogLevel MinimumLevel = LogLevel::Trace);
    
    static bool Set_MinimumLevel(LogChannel Channel, LogLevel Level);
    
// the APIs to deliver a single string(ig. "Texture Loaded") to Logger 
public: // Core logging
    static bool Write(LogLevel Level, LogChannel Channel, std::string_view Message, const codeplace& Location = codeplace::current());
    
    static bool Write(LogLevel Level, LogChannel Channel, std::wstring_view Message, const codeplace& Location = codeplace::current());
    
public: // Severity-specific APIs
    static bool Trace(LogChannel Channel, std::string_view Message, const codeplace& Location = codeplace::current());
    
    static bool Debug(LogChannel Channel, std::string_view Message, const codeplace& Location = codeplace::current());
    
    static bool Info(LogChannel Channel, std::string_view Message, const codeplace& Location = codeplace::current());
    
    static bool Warning(LogChannel Channel, std::string_view Message, const codeplace& Location = codeplace::current());
    
    static bool Error(LogChannel Channel, std::string_view Message, const codeplace& Location = codeplace::current());
    
    static bool Critical(LogChannel Channel, std::string_view Message, const codeplace& Location = codeplace::current());
    
public: // Default channel overloads
    static bool Trace(std::string_view Message, codeplace Location = codeplace::current());
    
    static bool Debug(std::string_view Message, codeplace Location = codeplace::current());
    
    static bool Info(std::string_view Message, codeplace Location = codeplace::current());
    
    static bool Warning(std::string_view Message, codeplace Location = codeplace::current());
    
    static bool Error(std::string_view Message, codeplace Location = codeplace::current());
    
    static bool Critical(std::string_view Message, codeplace Location = codeplace::current());
    
public: // WideString Support
    static bool Trace(LogChannel Channel, std::wstring_view Message, const codeplace& Location = codeplace::current());
    static bool Debug(LogChannel Channel, std::wstring_view Message, const codeplace& Location = codeplace::current());
    static bool Info(LogChannel Channel, std::wstring_view Message, const codeplace& Location = codeplace::current());
    static bool Warning(LogChannel Channel, std::wstring_view Message, const codeplace& Location = codeplace::current());
    static bool Error(LogChannel Channel, std::wstring_view Message, const codeplace& Location = codeplace::current());
    static bool Critical(LogChannel Channel, std::wstring_view Message, const codeplace& Location = codeplace::current());

public: // WideString Support
    static bool Trace(std::wstring_view Message, const codeplace& Location = codeplace::current());
    static bool Debug(std::wstring_view Message, const codeplace& Location = codeplace::current());
    static bool Info(std::wstring_view Message, const codeplace& Location = codeplace::current());
    static bool Warning(std::wstring_view Message, const codeplace& Location = codeplace::current());
    static bool Error(std::wstring_view Message, const codeplace& Location = codeplace::current());
    static bool Critical(std::wstring_view Message, const codeplace& Location = codeplace::current());
    
// the APIs to deliver a string with argument(ig. "Texture: {} ({}x{})", Name, Width, Height) to Logger
public: // template argument function  overload with Channel
    template<typename... Args> requires (sizeof...(Args) > 0)
    static bool Trace(LogChannel Channel, LogFormat<std::type_identity_t<Args>...> Format, Args&&... Arguments) {
        return Write_Formatted(LogLevel::Trace, Channel, Format, std::forward<Args>(Arguments)...);
    }
    
    template<typename... Args> requires (sizeof...(Args) > 0)
    static bool Debug(LogChannel Channel, LogFormat<std::type_identity_t<Args>...> Format, Args&&... Arguments) {
        return Write_Formatted(LogLevel::Debug, Channel, Format, std::forward<Args>(Arguments)...);
    }
    
    template<typename... Args> requires (sizeof...(Args) > 0)
    static bool Info(LogChannel Channel, LogFormat<std::type_identity_t<Args>...> Format, Args&&... Arguments) {
        return Write_Formatted(LogLevel::Info, Channel, Format, std::forward<Args>(Arguments)...);
    }
    
    template<typename... Args> requires (sizeof...(Args) > 0)
    static bool Warning(LogChannel Channel, LogFormat<std::type_identity_t<Args>...> Format, Args&&... Arguments) {
        return Write_Formatted(LogLevel::Warning, Channel, Format, std::forward<Args>(Arguments)...);
    }
    
    template<typename... Args> requires (sizeof...(Args) > 0)
    static bool Error(LogChannel Channel, LogFormat<std::type_identity_t<Args>...> Format, Args&&... Arguments) {
        return Write_Formatted(LogLevel::Error, Channel, Format, std::forward<Args>(Arguments)...);
    }
    
    template<typename... Args> requires (sizeof...(Args) > 0)
    static bool Critical(LogChannel Channel, LogFormat<std::type_identity_t<Args>...> Format, Args&&... Arguments) {
        return Write_Formatted(LogLevel::Critical, Channel, Format, std::forward<Args>(Arguments)...);
    }
    
public: // template argument function overload without Channel
    template<typename... Args> requires (sizeof...(Args) > 0)
    static bool Trace(LogFormat<std::type_identity_t<Args>...> Format, Args&&... Arguments) {
        return Trace(LogChannels::Engine, Format, std::forward<Args>(Arguments)...);
    }
    
    template<typename... Args> requires (sizeof...(Args) > 0)
    static bool Debug(LogFormat<std::type_identity_t<Args>...> Format, Args&&... Arguments) {
        return Debug(LogChannels::Engine, Format, std::forward<Args>(Arguments)...);
    }
    
    template<typename... Args> requires (sizeof...(Args) > 0)
    static bool Info(LogFormat<std::type_identity_t<Args>...> Format, Args&&... Arguments) {
        return Info(LogChannels::Engine, Format, std::forward<Args>(Arguments)...);
    }
    
    template<typename... Args> requires (sizeof...(Args) > 0)
    static bool Warning(LogFormat<std::type_identity_t<Args>...> Format, Args&&... Arguments) {
        return Warning(LogChannels::Engine, Format, std::forward<Args>(Arguments)...);
    }
    
    template<typename... Args> requires (sizeof...(Args) > 0)
    static bool Error(LogFormat<std::type_identity_t<Args>...> Format, Args&&... Arguments) {
        return Error(LogChannels::Engine, Format, std::forward<Args>(Arguments)...);
    }
    
    template<typename... Args> requires (sizeof...(Args) > 0)
    static bool Critical(LogFormat<std::type_identity_t<Args>...> Format, Args&&... Arguments) {
        return Critical(LogChannels::Engine, Format, std::forward<Args>(Arguments)...);
    }
    
public: // output configuration
    static void Enable_FileOutput(bool Enabled);
    
    static void Enable_DebugOutput(bool Enabled);

    static void Set_DebugMinimumLevel(LogLevel Level);
    
    static void Set_FileFlushPolicy(LogFlushPolicy Policy);
    
public:
    static bool Flush_All();
    
private:
    template<typename... Args>
    static bool Write_Formatted(LogLevel Level, LogChannel Channel, LogFormat<std::type_identity_t<Args>...> Format, 
        Args&&... Arguments) {
        const LogDecision Decision = Query_Log(Level, Channel);

        if (Decision == LogDecision::Invalid)  return false;
        if (Decision == LogDecision::Filtered) return true;

        return Write_Record(Level, Channel, std::format(Format.m_Text, std::forward<Args>(Arguments)...), Format.m_Location);
    } 
    
    static LogDecision Query_Log(LogLevel Level, LogChannel Channel);

    static bool Write_Record(LogLevel Level,LogChannel Channel, std::string Message, const codeplace& Location);
};
ENGINE_END
