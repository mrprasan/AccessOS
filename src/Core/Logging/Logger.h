// AccessOS/src/Core/Logging/Logger.h
//
// Structured logger for the AccessOS core.
//
// Why: All components must log through a single interface so that:
//   - Log destinations are swappable (file, ETW, debug output).
//   - Sensitive data can be filtered at one point before writing.
//   - Diagnostic and developer modes can be toggled at runtime.
//
// Threading: Log() is thread-safe. The underlying sink implementation
//            is responsible for serialization.
//
// Privacy: This interface does NOT accept arbitrary user content.
//          Callers must not pass passwords, PINs, or sensitive input
//          to any log method. See docs/privacy/13_Privacy.md.

#pragma once

#include <string>
#include <string_view>
#include <memory>

namespace AccessOS {

// Log severity levels.
enum class LogLevel {
    Debug,       // Detailed developer tracing — disabled in normal builds.
    Info,        // Normal operational events.
    Warning,     // Unexpected but recoverable conditions.
    Error,       // Failures that affect functionality.
    Critical,    // Failures that may require process restart.
};

// Structured log entry passed to sinks.
struct LogEntry {
    LogLevel        level;
    std::string     component;   // e.g. "UIAProvider", "SpeechEngine"
    std::string     message;
    std::string     file;        // __FILE__ — stripped in release if desired
    int             line;        // __LINE__
    // No timestamp here — sinks add timestamps to avoid clock calls on hot paths.
};

// Sink interface — implement to redirect log output.
class ILogSink {
public:
    virtual ~ILogSink() = default;
    virtual void Write(const LogEntry& entry) = 0;
};

// Central logger — one instance per process.
// Obtain via Logger::Instance().
class Logger {
public:
    static Logger& Instance() noexcept;

    // Set the minimum level to record. Messages below this level are discarded.
    void SetLevel(LogLevel level) noexcept;
    LogLevel GetLevel() const noexcept;

    // Replace the active sink. Pass nullptr to discard all output.
    // Not thread-safe with concurrent Log() calls — call during initialization only.
    void SetSink(std::shared_ptr<ILogSink> sink);

    // Core log method. Thread-safe.
    void Log(LogLevel level,
             std::string_view component,
             std::string_view message,
             const char* file,
             int line);

    // Convenience wrappers invoked through macros below.
    void Debug   (std::string_view component, std::string_view msg, const char* f, int l);
    void Info    (std::string_view component, std::string_view msg, const char* f, int l);
    void Warning (std::string_view component, std::string_view msg, const char* f, int l);
    void Error   (std::string_view component, std::string_view msg, const char* f, int l);
    void Critical(std::string_view component, std::string_view msg, const char* f, int l);

private:
    Logger();
    ~Logger() = default;

    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;

    LogLevel                    m_level{ LogLevel::Info };
    std::shared_ptr<ILogSink>   m_sink;
};

} // namespace AccessOS

// ─── Logging macros ──────────────────────────────────────────────────────────
// Use these throughout the codebase rather than calling Logger directly.
// The component string identifies the subsystem — use the class name.

#define ACOS_LOG_DEBUG(component, msg) \
    ::AccessOS::Logger::Instance().Debug((component), (msg), __FILE__, __LINE__)

#define ACOS_LOG_INFO(component, msg) \
    ::AccessOS::Logger::Instance().Info((component), (msg), __FILE__, __LINE__)

#define ACOS_LOG_WARNING(component, msg) \
    ::AccessOS::Logger::Instance().Warning((component), (msg), __FILE__, __LINE__)

#define ACOS_LOG_ERROR(component, msg) \
    ::AccessOS::Logger::Instance().Error((component), (msg), __FILE__, __LINE__)

#define ACOS_LOG_CRITICAL(component, msg) \
    ::AccessOS::Logger::Instance().Critical((component), (msg), __FILE__, __LINE__)
