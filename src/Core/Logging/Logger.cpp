// AccessOS/src/Core/Logging/Logger.cpp

#include "Logger.h"

#include <mutex>
#include <chrono>
#include <sstream>

// Debug output sink used as the default during development.
// Does not log sensitive content — callers are responsible for filtering.
#include <windows.h>

namespace AccessOS {

namespace {

// Default sink: writes to the Windows debugger output (OutputDebugStringW).
// Visible in Visual Studio Output window and tools like DebugView.
class DebugOutputSink final : public ILogSink {
public:
    void Write(const LogEntry& entry) override {
        // Format: [LEVEL][Component] message (file:line)
        static const char* levelNames[] = {
            "DEBUG", "INFO", "WARN", "ERROR", "CRITICAL"
        };

        const char* levelName = levelNames[static_cast<int>(entry.level)];

        std::ostringstream oss;
        oss << "[ACCESSOS][" << levelName << "]["
            << entry.component << "] "
            << entry.message;

        // Include source location only in debug builds.
#ifdef _DEBUG
        oss << " (" << entry.file << ":" << entry.line << ")";
#endif
        oss << "\n";

        // OutputDebugStringA is safe to call from any thread.
        ::OutputDebugStringA(oss.str().c_str());
    }
};

} // anonymous namespace

Logger& Logger::Instance() noexcept {
    // Guaranteed single instance — Meyer's singleton.
    static Logger instance;
    return instance;
}

Logger::Logger()
    : m_sink(std::make_shared<DebugOutputSink>())
{
}

void Logger::SetLevel(LogLevel level) noexcept {
    m_level = level;
}

LogLevel Logger::GetLevel() const noexcept {
    return m_level;
}

void Logger::SetSink(std::shared_ptr<ILogSink> sink) {
    // Caller must not call this concurrently with Log().
    m_sink = std::move(sink);
}

void Logger::Log(LogLevel level,
                 std::string_view component,
                 std::string_view message,
                 const char* file,
                 int line)
{
    if (level < m_level) return;

    auto sink = m_sink; // Copy shared_ptr — safe for concurrent reads.
    if (!sink) return;

    LogEntry entry;
    entry.level     = level;
    entry.component = std::string(component);
    entry.message   = std::string(message);
    entry.file      = file ? file : "";
    entry.line      = line;

    sink->Write(entry);
}

void Logger::Debug   (std::string_view c, std::string_view m, const char* f, int l) { Log(LogLevel::Debug,    c, m, f, l); }
void Logger::Info    (std::string_view c, std::string_view m, const char* f, int l) { Log(LogLevel::Info,     c, m, f, l); }
void Logger::Warning (std::string_view c, std::string_view m, const char* f, int l) { Log(LogLevel::Warning,  c, m, f, l); }
void Logger::Error   (std::string_view c, std::string_view m, const char* f, int l) { Log(LogLevel::Error,    c, m, f, l); }
void Logger::Critical(std::string_view c, std::string_view m, const char* f, int l) { Log(LogLevel::Critical, c, m, f, l); }

} // namespace AccessOS
