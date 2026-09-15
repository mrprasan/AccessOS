// AccessOS/src/Core/Diagnostics/DiagnosticsManager.cpp

#include "DiagnosticsManager.h"

namespace AccessOS {

DiagnosticsManager::DiagnosticsManager() = default;

DiagnosticsManager::~DiagnosticsManager() {
    CloseLogFile();
}

// ── Log file ──────────────────────────────────────────────────────────────────

bool DiagnosticsManager::SetLogFile(const std::string& path) {
    auto sink = std::make_shared<FileSink>();
    if (!sink->Open(path)) return false;

    m_fileSink = sink;
    Logger::Instance().SetSink(m_fileSink);
    return true;
}

void DiagnosticsManager::CloseLogFile() {
    if (m_fileSink) {
        m_fileSink->Close();
        m_fileSink.reset();
        // Revert logger to the default DebugOutput sink by passing nullptr
        // (Logger will discard entries) — caller can set a new sink afterwards.
        Logger::Instance().SetSink(nullptr);
    }
}

bool DiagnosticsManager::IsLoggingToFile() const noexcept {
    return m_fileSink && m_fileSink->IsOpen();
}

// ── Counters ──────────────────────────────────────────────────────────────────

DiagnosticsSnapshot DiagnosticsManager::GetSnapshot() const noexcept {
    const auto& c = DiagnosticsCounters::Instance();
    DiagnosticsSnapshot s;
    s.eventsProcessed  = c.EventsProcessed();
    s.focusChanges     = c.FocusChanges();
    s.speechUtterances = c.SpeechUtterances();
    s.contextSwitches  = c.ContextSwitches();
    s.speechCancels    = c.SpeechCancels();
    s.uptimeMs         = c.UptimeMs();
    return s;
}

void DiagnosticsManager::ResetCounters() noexcept {
    DiagnosticsCounters::Instance().Reset();
}

// ── Static convenience wrappers ───────────────────────────────────────────────

void DiagnosticsManager::RecordEvent()         noexcept { DiagnosticsCounters::Instance().IncrementEventsProcessed(); }
void DiagnosticsManager::RecordFocusChange()   noexcept { DiagnosticsCounters::Instance().IncrementFocusChanges(); }
void DiagnosticsManager::RecordSpeech()        noexcept { DiagnosticsCounters::Instance().IncrementSpeechUtterances(); }
void DiagnosticsManager::RecordContextSwitch() noexcept { DiagnosticsCounters::Instance().IncrementContextSwitches(); }
void DiagnosticsManager::RecordSpeechCancel()  noexcept { DiagnosticsCounters::Instance().IncrementSpeechCancels(); }

} // namespace AccessOS
