// AccessOS/src/Core/Diagnostics/DiagnosticsCounters.h
//
// DiagnosticsCounters — atomic runtime performance counters.
//
// Why: The UI shell and diagnostics tooling need live metrics without
//      locking any subsystem. All counters are std::atomic — safe to
//      increment from any thread and read from any thread.
//
// Usage: Call DiagnosticsCounters::Instance() to get the singleton,
//        then call IncrementXxx() from the relevant subsystem.

#pragma once

#include <atomic>
#include <cstdint>
#include <chrono>

namespace AccessOS {

class DiagnosticsCounters {
public:
    /// Meyer's singleton — one set of counters per process.
    static DiagnosticsCounters& Instance() noexcept;

    // ── Increment (called by subsystems) ─────────────────────────────────────

    void IncrementEventsProcessed()  noexcept { ++m_eventsProcessed; }
    void IncrementFocusChanges()     noexcept { ++m_focusChanges; }
    void IncrementSpeechUtterances() noexcept { ++m_speechUtterances; }
    void IncrementContextSwitches()  noexcept { ++m_contextSwitches; }
    void IncrementSpeechCancels()    noexcept { ++m_speechCancels; }

    // ── Read ─────────────────────────────────────────────────────────────────

    uint64_t EventsProcessed()  const noexcept { return m_eventsProcessed.load(); }
    uint64_t FocusChanges()     const noexcept { return m_focusChanges.load(); }
    uint64_t SpeechUtterances() const noexcept { return m_speechUtterances.load(); }
    uint64_t ContextSwitches()  const noexcept { return m_contextSwitches.load(); }
    uint64_t SpeechCancels()    const noexcept { return m_speechCancels.load(); }

    /// Wall-clock uptime in milliseconds since Reset() or construction.
    uint64_t UptimeMs() const noexcept;

    // ── Reset ─────────────────────────────────────────────────────────────────

    void Reset() noexcept;

private:
    DiagnosticsCounters() noexcept;

    std::atomic<uint64_t> m_eventsProcessed{ 0 };
    std::atomic<uint64_t> m_focusChanges{ 0 };
    std::atomic<uint64_t> m_speechUtterances{ 0 };
    std::atomic<uint64_t> m_contextSwitches{ 0 };
    std::atomic<uint64_t> m_speechCancels{ 0 };

    std::chrono::steady_clock::time_point m_startTime;
};

} // namespace AccessOS
