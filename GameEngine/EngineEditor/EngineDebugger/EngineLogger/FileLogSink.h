#pragma once
#include <filesystem>
#include <fstream>

#include "Engine_Default.h"
#include "Engine_Interface.h"

ENGINE_START
    enum class LogFileMode : std::uint8_t { Append, Truncate };

class ENGINE_DLL FileLogSink final : public Interface::Uncopyable {
public:
    [[nodiscard("Check Log File opened properly.")]]
    bool Open(const std::filesystem::path& FilePath, LogFileMode Mode = LogFileMode::Append);
    
    [[nodiscard("Check Log File wrote messages properly.")]]
    bool Write(std::string_view Message);
    
    [[nodiscard("Check Log File flushed properly.")]]
    bool Flush();
    
    [[nodiscard("Check Log File closed properly.")]]
    bool Close();

    [[nodiscard("Check Log File opened properly.")]]
    bool Is_Open() const noexcept;
    
private:
    std::ofstream m_Stream;
};

ENGINE_END