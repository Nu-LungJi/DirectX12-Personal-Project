#pragma once
#include <filesystem>

#include "Engine_Default.h"
#include "Engine_Interface.h"

#include "FileLogSink.h"
#include "DebugLogSink.h"
#include "LogTypeLookUp.h"

ENGINE_START
    // LoggerService : Classify logs by LogRecord Information and deliver to FilelogSink
class ENGINE_DLL LoggerService final : public Interface::Uncopyable, public Interface::Singleton<LoggerService>  {
public:
    ~LoggerService();
public:
    [[nodiscard]]
    bool Initialize(const std::filesystem::path& RootDirectory);
    
    [[nodiscard]]
    bool Shutdown();
    
public:
    [[nodiscard]]
    bool Register_Channel(LogChannel Channel, std::string_view Name, const std::filesystem::path& FilePath,
            LogFileMode Mode = LogFileMode::Append, LogLevel MinimumLevel = LogLevel::Trace);
    
    [[nodiscard]]
    bool Set_MinimumLevel(LogChannel Channel, LogLevel Level);

    [[nodiscard]]
    bool Should_Log(LogLevel Level, LogChannel Channel) const;

    [[nodiscard]]
    bool Write(const LogRecord& Record);

    [[nodiscard]]
    bool Flush_All();
    
public:
    // Configures File output.
    void Enable_FileOutput(bool Enabled);
    
    // Configures debugger output.
    void Enable_DebugOutput(bool Enabled);

    // Sets the minimum severity for debugger output.
    void Set_DebugMinimumLevel(LogLevel Level);
    
    void Set_FileFlushPolicy(LogFlushPolicy Policy);
    
    [[nodiscard]]
    LogDecision Query_Log(LogLevel Level, LogChannel Channel) const;
    
private:
    struct ChannelState final {
        std::string m_Name;
        LogLevel m_MinimumLevel = LogLevel::Trace;
        FileLogSink m_File;
    };
    
    struct OutputTargets final {
        bool File = false;
        bool Debug = false;

        constexpr bool Any() const noexcept { return File || Debug; }
    };
    
private:
    [[nodiscard]]
    OutputTargets Evaluate_Targets(LogLevel Level, const ChannelState& State) const noexcept;
    
    [[nodiscard]]
    static constexpr bool Should_Flush(LogLevel Level, LogFlushPolicy Policy) noexcept;
    
private:
    mutable std::mutex m_Mutex;

    bool m_Initialized = false;

    std::filesystem::path m_RootDirectory;

    std::unordered_map<std::uint32_t, ChannelState> m_ChannelList;
    
private:
    DebugLogSink m_DebugSink;

    bool m_FileOutputEnabled = true;    // Policy : File Output always true(Default), even if Release Mode
    
#if defined(_DEBUG) || defined(_PROFILE)
    bool m_DebugOutputEnabled = true;
#else
    bool m_DebugOutputEnabled = false;
#endif

    LogLevel m_DebugMinimumLevel = LogLevel::Warning;
    
private:
    LogFlushPolicy m_FileFlushPolicy = LogFlushPolicy::OnCritical;
};


ENGINE_END
