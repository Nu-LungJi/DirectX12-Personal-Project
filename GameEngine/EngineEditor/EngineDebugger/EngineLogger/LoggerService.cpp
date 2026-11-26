#include "LoggerService.h"
ENGINE_USE

namespace
{
    constexpr std::string_view Get_LevelName(ENGINE::LogLevel Level) noexcept {
        switch (Level) {
            case LogLevel::Trace:    return "TRACE";
            case LogLevel::Debug:    return "DEBUG";
            case LogLevel::Info:     return "INFO";
            case LogLevel::Warning:  return "WARNING";
            case LogLevel::Error:    return "ERROR";
            case LogLevel::Critical: return "CRITICAL";
    
            default: return "UNKNOWN";
        }
    }

    std::string Format_Record(const ENGINE::LogRecord& Record, std::string_view ChannelName) {
        const std::time_t Seconds = std::chrono::system_clock::to_time_t(Record.m_LogTimestamp);

        std::tm LocalTime{};

        const bool ValidTime = localtime_s(&LocalTime, &Seconds) == 0;

        std::ostringstream Stream;
        
        const auto Milliseconds 
            = std::chrono::duration_cast<std::chrono::milliseconds>(Record.m_LogTimestamp.time_since_epoch()).count() % 1000;

        if (ValidTime)
        {
            Stream << '[' << std::put_time(&LocalTime, "%Y-%m-%d %H:%M:%S") 
            << '.' << std::setw(3) << std::setfill('0') << Milliseconds << ']';
        }
        else { Stream << "[TIME_UNAVAILABLE]"; }

        Stream
            << '[' << Get_LevelName(Record.m_LogLevel) << ']'
            << '[' << ChannelName << ']'
            << "[Thread: " << Record.m_ThreadID << "]\n";

        if (Record.m_Location.line() != 0) {
            Stream << "  Source  : " << Record.m_Location.file_name() << ':' << Record.m_Location.line() << '\n';
        }
        Stream << "  Message : " << Record.m_LogMessage;

        if (Record.m_LogMessage.empty() || Record.m_LogMessage.back() != '\n') Stream << '\n';

        return Stream.str();
    }
    // ========================== Log Example ==========================
    // [2026-11-26 14:30:12.125][ERROR][Graphics][Thread: 10432]
    // Source  : GraphicDevice.cpp:245
    // Message : D3D12 device creation failed.
}

LoggerService::~LoggerService() { (void)Shutdown(); }

bool LoggerService::Initialize(const std::filesystem::path& RootDirectory) {
    std::scoped_lock Lock(m_Mutex);

    if (m_Initialized || RootDirectory.empty()) return false;

    std::error_code Error;
    std::filesystem::create_directories(RootDirectory, Error);
    if (Error) return false;

    m_RootDirectory = RootDirectory;
    m_Initialized = true;

    return true;
}

bool LoggerService::Shutdown() {
    std::scoped_lock Lock(m_Mutex);

    if (!m_Initialized) return true;

    bool Success = true;

    for (auto& [ID, State] : m_ChannelList) {
        if (!State.m_File.Close()) Success = false;
    }

    m_ChannelList.clear();
    m_RootDirectory.clear();
    m_Initialized = false;

    return Success;
}

bool LoggerService::Register_Channel(LogChannel Channel, std::string_view Name, const std::filesystem::path& FilePath,
    LogFileMode Mode, LogLevel MinimumLevel) {
    std::scoped_lock Lock(m_Mutex);

    if (!m_Initialized || Name.empty() || FilePath.empty()) return false;

    // Keep registered log files inside the configured directory.
    if (FilePath.is_absolute() || FilePath.has_root_name() || FilePath.has_root_directory()) return false;

    for (const auto& Part : FilePath) {
        if (Part == "..") return false;
    }

    // Construct ChannelState directly inside the map.
    auto [It, Inserted] = m_ChannelList.try_emplace(Channel.ChannelID);

    if (!Inserted) return false;

    ChannelState& State = It->second;

    State.m_Name = std::string(Name);
    State.m_MinimumLevel = MinimumLevel;

    const auto FullPath = m_RootDirectory / FilePath;

    if (!State.m_File.Open(FullPath, Mode)) {
        m_ChannelList.erase(It);
        return false;
    }

    return true;
}

bool LoggerService::Set_MinimumLevel(LogChannel Channel, LogLevel Level) {
    std::scoped_lock Lock(m_Mutex);

    auto It = m_ChannelList.find(Channel.ChannelID);

    if (It == m_ChannelList.end())
        return false;

    It->second.m_MinimumLevel = Level;

    return true;
}

bool LoggerService::Should_Log(LogLevel Level, LogChannel Channel) const {
    return Query_Log(Level, Channel) == LogDecision::Accepted;
}

bool LoggerService::Write(const LogRecord& Record) {
    std::scoped_lock Lock(m_Mutex);

    if (!m_Initialized) return false;

    const auto ChannelIter = m_ChannelList.find(Record.m_LogChannel.ChannelID);
    if (ChannelIter == m_ChannelList.end()) return false;

    ChannelState& State = ChannelIter->second;
    
    // Determine the output destinations.
    const OutputTargets Targets = Evaluate_Targets(Record.m_LogLevel, State);
    
    // Skip formatting when no sink accepts the message.
    if (!Targets.Any()) return true;

    // Format the message only once.
    const std::string Formatted = Format_Record(Record, State.m_Name);

    bool Success = true;

    // Write independently to each enabled sink.
    if (Targets.File) {
        const bool WriteResult = State.m_File.Write(Formatted);

        if (!WriteResult) Success = false;
        else if (Should_Flush(Record.m_LogLevel, m_FileFlushPolicy))
            if (!State.m_File.Flush()) Success = false;
    }

    if (Targets.Debug) {
        if (!m_DebugSink.Write(Formatted)) Success = false;
    }

    return Success;
}

bool LoggerService::Flush_All() {
    std::scoped_lock Lock(m_Mutex);

    if (!m_Initialized) return false;

    bool Success = true;

    for (auto& [ID, State] : m_ChannelList) {
        if (!State.m_File.Flush()) Success = false;
    }

    return Success;
}

void LoggerService::Enable_FileOutput(bool Enabled) {
    std::scoped_lock Lock(m_Mutex);
    m_FileOutputEnabled = Enabled;
}

void LoggerService::Enable_DebugOutput(bool Enabled) {
    std::scoped_lock Lock(m_Mutex);
    m_DebugOutputEnabled = Enabled;
}

void LoggerService::Set_DebugMinimumLevel(LogLevel Level) {
    std::scoped_lock Lock(m_Mutex);
    m_DebugMinimumLevel = Level;
}

void LoggerService::Set_FileFlushPolicy(LogFlushPolicy Policy) {
    std::scoped_lock Lock(m_Mutex);
    m_FileFlushPolicy = Policy;
}

constexpr bool LoggerService::Should_Flush(LogLevel Level, LogFlushPolicy Policy) noexcept {
    switch (Policy) {
        case LogFlushPolicy::Manual:        return false;
        case LogFlushPolicy::OnCritical:    return Level >= LogLevel::Critical;
        case LogFlushPolicy::OnError:       return Level >= LogLevel::Error;
        case LogFlushPolicy::Always:        return true;
        default:                            return false;
    }
}

LogDecision LoggerService::Query_Log(LogLevel Level, LogChannel Channel) const {
    std::scoped_lock Lock(m_Mutex);

    if (!m_Initialized) return LogDecision::Invalid;

    const auto It = m_ChannelList.find(Channel.ChannelID);
    if (It == m_ChannelList.end()) return LogDecision::Invalid;

    const OutputTargets Targets = Evaluate_Targets(Level, It->second);

    return Targets.Any() ? LogDecision::Accepted : LogDecision::Filtered;
}

LoggerService::OutputTargets LoggerService::Evaluate_Targets(LogLevel Level, const ChannelState& State) const noexcept {
    return {
        .File  = m_FileOutputEnabled  && Level >= State.m_MinimumLevel,
        .Debug = m_DebugOutputEnabled && Level >= m_DebugMinimumLevel
    };
}

