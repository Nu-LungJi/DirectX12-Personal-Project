#include "FileLogSink.h"
ENGINE_USE

bool FileLogSink::Open(const std::filesystem::path& FilePath, LogFileMode Mode) {
    if (FilePath.empty()) return false;
    
    const std::filesystem::path Parent = FilePath.parent_path();    // stored Directory path
    
    // Create parent directories if necessary.
    if (!Parent.empty()) {
        std::error_code Error;
        std::filesystem::create_directories(Parent, Error);
        if (Error) return false;
    }
    // Close the previous file before opening a new one.
    if (!Close()) return false;
    
    // LogFileMode::Append : Keep the existing log and append to it.
    // LogFileMode::Truncate : Clear existing log content and record new.
    
    std::ios::openmode OpenMode = std::ios::out | std::ios::binary;
    OpenMode |= (Mode == LogFileMode::Append) ? std::ios::app : std::ios::trunc;
    
    m_Stream.open(FilePath, OpenMode);
    return m_Stream.is_open() && m_Stream.good();
}

bool FileLogSink::Write(std::string_view Message) {
    if (!m_Stream.is_open() || !m_Stream.good()) return false;
    
    if (Message.empty()) return true;
    
    if (Message.size() > static_cast<std::size_t>((std::numeric_limits<std::streamsize>::max)())) return false;
    
    const std::streamsize MessageSize = static_cast<std::streamsize>(Message.size());
    
    m_Stream.write(Message.data(), MessageSize);
    
    return m_Stream.good();
}

bool FileLogSink::Flush() {
    if (!m_Stream.is_open()) return false;

    m_Stream.flush();

    return m_Stream.good();
}

bool FileLogSink::Close() {
    if (!m_Stream.is_open()) {
        m_Stream.clear();
        return true;
    }

    m_Stream.close();

    const bool Result = !m_Stream.fail();

    m_Stream.clear();

    return Result;
}

bool FileLogSink::Is_Open() const noexcept {
    return m_Stream.is_open();
}
