#include "Aemeath/Core/Logging/Logger.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace Aemeath::Core::Logging {

namespace {

std::mutex g_mutex;
std::ofstream g_stream;
LogLevel g_minimum { LogLevel::Info };
std::filesystem::path g_path;

std::string FormatTimestamp() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto seconds = system_clock::to_time_t(now);
    const auto millis = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::tm local {};
#ifdef _WIN32
    localtime_s(&local, &seconds);
#else
    localtime_r(&seconds, &local);
#endif

    char buffer[64] {};
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%04d-%02d-%02d %02d:%02d:%02d.%03lld",
        local.tm_year + 1900,
        local.tm_mon + 1,
        local.tm_mday,
        local.tm_hour,
        local.tm_min,
        local.tm_sec,
        static_cast<long long>(millis.count()));
    return buffer;
}

} // namespace

std::string_view ToString(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Trace: return "TRACE";
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Warn: return "WARN";
    case LogLevel::Error: return "ERROR";
    }
    return "INFO";
}

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    Flush();
}

void Logger::SetMinimumLevel(LogLevel level) noexcept {
    std::lock_guard lock(g_mutex);
    g_minimum = level;
}

LogLevel Logger::MinimumLevel() const noexcept {
    std::lock_guard lock(g_mutex);
    return g_minimum;
}

void Logger::SetLogFile(std::filesystem::path path) {
    std::lock_guard lock(g_mutex);
    if (g_stream.is_open()) {
        g_stream.flush();
        g_stream.close();
    }
    g_path = std::move(path);

    if (g_path.empty()) {
        return;
    }

    std::error_code ec;
    const auto parent = g_path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, ec);
    }
    g_stream.open(g_path, std::ios::app | std::ios::binary);
}

void Logger::Write(LogLevel level, std::string_view category, std::string_view message) {
    {
        std::lock_guard lock(g_mutex);
        if (static_cast<int>(level) < static_cast<int>(g_minimum)) {
            return;
        }
    }

    std::string line;
    line.reserve(category.size() + message.size() + 48);
    line += FormatTimestamp();
    line += " [";
    line += ToString(level);
    line += "] [";
    line += category;
    line += "] ";
    line += message;
    line += '\n';

#ifdef _WIN32
    ::OutputDebugStringA(line.c_str());
#endif

    std::lock_guard lock(g_mutex);
    if (g_stream.is_open()) {
        g_stream << line;
        g_stream.flush();
    }
}

void Logger::Flush() {
    std::lock_guard lock(g_mutex);
    if (g_stream.is_open()) {
        g_stream.flush();
    }
}

} // namespace Aemeath::Core::Logging