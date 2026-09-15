// AccessOS/src/Core/Diagnostics/FileSink.h
//
// FileSink — ILogSink implementation that appends structured log entries
//             to a UTF-8 text file.
//
// Why: In production the debug-output sink is invisible to users.
//      A file sink lets support staff collect logs without attaching a debugger.
//
// Format per line:  [HH:MM:SS.mmm][LEVEL][Component] message
//
// Threading: Write() is serialized via mutex — safe from any thread.
//            Open() / Close() must not be called concurrently with Write().

#pragma once

#include "../Logging/Logger.h"
#include <fstream>
#include <mutex>
#include <string>

namespace AccessOS {

class FileSink final : public ILogSink {
public:
    FileSink() = default;
    ~FileSink() override { Close(); }

    /// Open the log file at path (creates or appends).
    /// Returns true on success.
    bool Open(const std::string& path);

    /// Close the file. Subsequent Write() calls are silently ignored.
    void Close();

    bool IsOpen() const noexcept;

    /// ILogSink — serialize and append one log entry.
    void Write(const LogEntry& entry) override;

private:
    mutable std::mutex  m_mutex;
    std::ofstream       m_file;
    bool                m_open{ false };
};

} // namespace AccessOS
