// AccessOS/src/Core/Diagnostics/DiagnosticsCounters.cpp

#include "DiagnosticsCounters.h"

namespace AccessOS {

DiagnosticsCounters& DiagnosticsCounters::Instance() noexcept {
    static DiagnosticsCounters instance;
    return instance;
}

DiagnosticsCounters::DiagnosticsCounters() noexcept
    : m_startTime(std::chrono::steady_clock::now())
{
}

uint64_t DiagnosticsCounters::UptimeMs() const noexcept {
    const auto now = std::chrono::steady_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - m_startTime).count());
}

void DiagnosticsCounters::Reset() noexcept {
    m_eventsProcessed.store(0);
    m_focusChanges.store(0);
    m_speechUtterances.store(0);
    m_contextSwitches.store(0);
    m_speechCancels.store(0);
    m_startTime = std::chrono::steady_clock::now();
}

} // namespace AccessOS
