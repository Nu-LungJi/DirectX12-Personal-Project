#pragma once
#include <chrono>

ENGINE_START
    using TimePoint = std::chrono::system_clock::time_point;

// Represents the severity of a log message.
enum class LogLevel : std::uint8_t { Trace, Debug, Info, Warning, Error, Critical };

enum class LogDecision : std::uint8_t {
    Invalid,    // Logger unavailable or channel not registered
    Filtered,   // No sink accepts the message
    Accepted    // At least one sink accepts the message
};

enum class LogFlushPolicy : std::uint8_t {
    Manual,         // No automatic Flush
    OnCritical,     // Flush over Critical
    OnError,        // Flush over Error
    Always          // Flush everytime record the log on files
};

// Identifies the system that produces a log message.
struct LogChannel final {
    std::uint32_t ChannelID = 0;

    constexpr bool operator==(const LogChannel&) const noexcept = default;
};

// Built-in logging channels.
namespace LogChannels {
    inline constexpr LogChannel Engine      { 0 };
    inline constexpr LogChannel Graphics    { 1 };
    inline constexpr LogChannel Resource    { 2 };
    inline constexpr LogChannel Network     { 3 };
    inline constexpr LogChannel Thread      { 4 };
    inline constexpr LogChannel Diagnostics { 5 };
}

struct LogRecord final {
    LogLevel    m_LogLevel = LogLevel::Info;
    LogChannel  m_LogChannel = LogChannels::Engine;
    std::string m_LogMessage;
    
    TimePoint   m_LogTimestamp{};
    std::thread::id m_ThreadID{};         // the thread which makes log
    std::source_location m_Location{};    // the code location which makes log
};
ENGINE_END