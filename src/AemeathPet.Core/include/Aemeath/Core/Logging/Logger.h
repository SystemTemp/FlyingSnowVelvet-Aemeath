#pragma once

#include <filesystem>
#include <string>

namespace Aemeath::Core::Logging {

enum class LogLevel {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4
};

/// 进程级日志器：线程安全，写入 <root>/logs/aemeath.log，同时可选输出到调试器。
class Logger final {
public:
    static Logger& Instance();

    void SetMinimumLevel(LogLevel level) noexcept;
    [[nodiscard]] LogLevel MinimumLevel() const noexcept;

    /// 设置日志文件路径；空路径表示只写调试输出。
    void SetLogFile(std::filesystem::path path);

    void Write(LogLevel level, std::string_view category, std::string_view message);

    void Trace(std::string_view category, std::string_view message) { Write(LogLevel::Trace, category, message); }
    void Debug(std::string_view category, std::string_view message) { Write(LogLevel::Debug, category, message); }
    void Info(std::string_view category, std::string_view message) { Write(LogLevel::Info, category, message); }
    void Warn(std::string_view category, std::string_view message) { Write(LogLevel::Warn, category, message); }
    void Error(std::string_view category, std::string_view message) { Write(LogLevel::Error, category, message); }

    void Flush();

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

[[nodiscard]] std::string_view ToString(LogLevel level) noexcept;

} // namespace Aemeath::Core::Logging