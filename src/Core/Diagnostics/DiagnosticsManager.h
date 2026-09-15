// AccessOS/src/Core/Diagnostics/DiagnosticsManager.h
//
// DiagnosticsManager — aggregates counters and the file log sink.
//
// Why: A single object to open/close the log file and query all
//      runtime metrics.  The DLL export layer holds one instance
//      per CoreInstance.
//
// Threading: All public methods are thread-safe.

#pragma once

#include "DiagnosticsCounters.h"
#include "FileSink.h"
#include "../Logging/Logger.h"

#include <memory>
#include <string>

namespace AccessOS {

/// A snapshot of all diagnostic counters at one point in time.
struct DiagnosticsSnapshot {
    uint64_t eventsProcessed  = 0;
    uint64_t focusChanges     = 0;
    uint64_t speechUtterances = 0;
    uint64_t contextSwitches  = 0;
    uint64_t speechCancels    = 0;
    uint64_t uptimeMs         = 0;
};

class DiagnosticsManager {
public:
    DiagnosticsManager();
    ~DiagnosticsManager();

    // Not copyable.
    DiagnosticsManager(const DiagnosticsManager&)            = delete;
    DiagnosticsManager& operator=(const DiagnosticsManager&) = delete;

    // ── Log file ─────────────────────────────────────────────────────────────

    /// Open a log file at path.  Replaces any previously installed sink.
    /// Returns true on success.
    bool SetLogFile(const std::string& path);

    /// Close the log file and revert to DebugOutput sink.
    void CloseLogFile();

    bool IsLoggingToFile() const noexcept;

    // ── Counters ─────────────────────────────────────────────────────────────

    /// Get a snapshot of all current counters.
    DiagnosticsSnapshot GetSnapshot() const noexcept;

    /// Reset all counters to zero and restart the uptime clock.
    void ResetCounters() noexcept;

    // ── Convenience counter accessors (delegates to singleton) ───────────────

    static void RecordEvent()          noexcept;
    static void RecordFocusChange()    noexcept;
    static void RecordSpeech()         noexcept;
    static void RecordContextSwitch()  noexcept;
    static void RecordSpeechCancel()   noexcept;

private:
    std::shared_ptr<FileSink> m_fileSink;
};

} // namespace AccessOS
