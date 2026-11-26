#include "EngineLogger.h"

#include "EngineUtility.h"
#include "LoggerService.h"
ENGINE_USE

bool EngineLogger::Initialize(const std::filesystem::path& RootDirectory) {
    if (!LoggerService::GetInstance().Initialize(RootDirectory)) return false;
    
    const bool RegisterResult = 
        LoggerService::GetInstance().Register_Channel(LogChannels::Engine, "Engine", "Engine.log") &&
        LoggerService::GetInstance().Register_Channel(LogChannels::Graphics, "Graphics", "Graphics.log") &&
        LoggerService::GetInstance().Register_Channel(LogChannels::Resource, "Resource", "Resource.log") &&
        LoggerService::GetInstance().Register_Channel(LogChannels::Thread, "Thread", "Thread.log") &&
        LoggerService::GetInstance().Register_Channel(LogChannels::Network, "Network", "Network.log") &&
        LoggerService::GetInstance().Register_Channel(LogChannels::Diagnostics, "Diagnostics", "Diagnostics.log");
    
    if (!RegisterResult) {
        (void)LoggerService::GetInstance().Shutdown();
        return false;
    }
    
    return true;
}

bool EngineLogger::Shutdown() {
    return LoggerService::GetInstance().Shutdown();
}

bool EngineLogger::Register_Channel(LogChannel Channel, std::string_view Name, const std::filesystem::path& FilePath,
    LogFileMode Mode, LogLevel MinimumLevel) {
    return LoggerService::GetInstance().Register_Channel(Channel, Name, FilePath, Mode, MinimumLevel);
}

bool EngineLogger::Set_MinimumLevel(LogChannel Channel, LogLevel Level) {
    return LoggerService::GetInstance().Set_MinimumLevel(Channel, Level);
}

bool EngineLogger::Write(LogLevel Level, LogChannel Channel, std::string_view Message, const codeplace& Location) {
    const LogDecision Decision = Query_Log(Level, Channel);

    if (Decision == LogDecision::Invalid)  return false;
    if (Decision == LogDecision::Filtered) return true;

    return Write_Record(Level, Channel, std::string(Message), Location );
}

bool EngineLogger::Write(LogLevel Level, LogChannel Channel, std::wstring_view Message, const codeplace& Location) {
    const LogDecision Decision = Query_Log(Level, Channel);

    if (Decision == LogDecision::Invalid)  return false;
    if (Decision == LogDecision::Filtered) return true;

    return Write_Record(Level, Channel, EngineUtility::Convert_NormalString(Message), Location );
}

bool EngineLogger::Trace(LogChannel Channel, std::string_view Message, const codeplace& Location) {
    return Write(LogLevel::Trace, Channel, Message, Location);
}

bool EngineLogger::Debug(LogChannel Channel, std::string_view Message, const codeplace& Location) {
    return Write(LogLevel::Debug, Channel, Message, Location);
}

bool EngineLogger::Info(LogChannel Channel, std::string_view Message, const codeplace& Location) {
    return Write(LogLevel::Info, Channel, Message, Location);
}

bool EngineLogger::Warning(LogChannel Channel, std::string_view Message, const codeplace& Location) {
    return Write(LogLevel::Warning, Channel, Message, Location);
}

bool EngineLogger::Error(LogChannel Channel, std::string_view Message, const codeplace& Location) {
    return Write(LogLevel::Error, Channel, Message, Location);
}

bool EngineLogger::Critical(LogChannel Channel, std::string_view Message, const codeplace& Location) {
    return Write(LogLevel::Critical, Channel, Message, Location);
}

bool EngineLogger::Trace(std::string_view Message, codeplace Location) {
    return Trace(LogChannels::Engine, Message, Location);
}

bool EngineLogger::Debug(std::string_view Message, codeplace Location) {
    return Debug(LogChannels::Engine, Message, Location);
}

bool EngineLogger::Info(std::string_view Message, codeplace Location) {
    return Info(LogChannels::Engine, Message, Location);
}

bool EngineLogger::Warning(std::string_view Message, codeplace Location) {
    return Warning(LogChannels::Engine, Message, Location);
}

bool EngineLogger::Error(std::string_view Message, codeplace Location) {
    return Error(LogChannels::Engine, Message, Location);
}

bool EngineLogger::Critical(std::string_view Message, codeplace Location) {
    return Critical(LogChannels::Engine, Message, Location);
}

bool EngineLogger::Trace(LogChannel Channel, std::wstring_view Message, const codeplace& Location) {
    return Write(LogLevel::Trace, Channel, Message, Location);
}

bool EngineLogger::Debug(LogChannel Channel, std::wstring_view Message, const codeplace& Location){
    return Write(LogLevel::Debug, Channel, Message, Location);
}

bool EngineLogger::Info(LogChannel Channel, std::wstring_view Message, const codeplace& Location){
    return Write(LogLevel::Info, Channel, Message, Location);
}

bool EngineLogger::Warning(LogChannel Channel, std::wstring_view Message, const codeplace& Location){
    return Write(LogLevel::Warning, Channel, Message, Location);
}

bool EngineLogger::Error(LogChannel Channel, std::wstring_view Message, const codeplace& Location){
    return Write(LogLevel::Error, Channel, Message, Location);
}

bool EngineLogger::Critical(LogChannel Channel, std::wstring_view Message, const codeplace& Location){
    return Write(LogLevel::Critical, Channel, Message, Location);
}

bool EngineLogger::Trace(std::wstring_view Message, const codeplace& Location) {
    return Trace(LogChannels::Engine, Message, Location);
}

bool EngineLogger::Debug(std::wstring_view Message, const codeplace& Location) {
    return Debug(LogChannels::Engine, Message, Location);
}

bool EngineLogger::Info(std::wstring_view Message, const codeplace& Location) {
    return Info(LogChannels::Engine, Message, Location);
}

bool EngineLogger::Warning(std::wstring_view Message, const codeplace& Location) {
    return Warning(LogChannels::Engine, Message, Location);
}

bool EngineLogger::Error(std::wstring_view Message, const codeplace& Location) {
    return Error(LogChannels::Engine, Message, Location);
}

bool EngineLogger::Critical(std::wstring_view Message, const codeplace& Location) {
    return Critical(LogChannels::Engine, Message, Location);
}

void EngineLogger::Enable_FileOutput(bool Enabled) {
    LoggerService::GetInstance().Enable_FileOutput(Enabled);
}

void EngineLogger::Enable_DebugOutput(bool Enabled) {
    LoggerService::GetInstance().Enable_DebugOutput(Enabled);
}

void EngineLogger::Set_DebugMinimumLevel(LogLevel Level) {
    LoggerService::GetInstance().Set_DebugMinimumLevel(Level);
}

void EngineLogger::Set_FileFlushPolicy(LogFlushPolicy Policy) {
    LoggerService::GetInstance().Set_FileFlushPolicy(Policy);
}

bool EngineLogger::Flush_All() {
    return LoggerService::GetInstance().Flush_All();
}

LogDecision EngineLogger::Query_Log(LogLevel Level, LogChannel Channel) {
    return LoggerService::GetInstance().Query_Log(Level, Channel);
}

bool EngineLogger::Write_Record(LogLevel Level, LogChannel Channel, std::string Message, const codeplace& Location) {
    LogRecord Record{
        .m_LogLevel     = Level,
        .m_LogChannel   = Channel,
        .m_LogMessage   = std::move(Message),
        .m_LogTimestamp = std::chrono::system_clock::now(),
        .m_ThreadID     = std::this_thread::get_id(),
        .m_Location     = Location
    };

    return LoggerService::GetInstance().Write(Record);
}