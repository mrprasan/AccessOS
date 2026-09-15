// AccessOS/src/Core/Reader/AccessReader.h
//
// AccessReader — the central coordinator of the AccessOS screen reader runtime.
//
// Why: Each subsystem (EventEngine, ContextEngine, FocusManager,
//      AnnouncementEngine, SpeechManager) is independent and testable in
//      isolation.  AccessReader is the single object that wires them together
//      and applies the event-routing policy:
//
//   FocusChanged   → ContextEngine::Update → AnnouncementEngine::BuildFocusAnnouncement
//                  → SpeechManager::SpeakAnnouncement
//   ValueChanged / StateChanged
//                  → AnnouncementEngine::BuildPropertyAnnouncement
//                  → SpeechManager::SpeakAnnouncement
//   LiveRegionChanged / Alert
//                  → AnnouncementEngine::BuildAlertAnnouncement
//                  → SpeechManager::SpeakAnnouncement  (High priority)
//
// Lifecycle:
//   1. Construct with injected subsystem pointers (non-owning).
//   2. Call Initialize() — registers self as an IEventListener.
//   3. Call Shutdown() before destroying any subsystem.
//
// Threading:
//   OnEvent() is called on the EventEngine worker thread.
//   All public control methods (SetVerbosity, Stop, …) are thread-safe.
//
// Must NOT:
//   - Query UIA directly.
//   - Log user content or element values (PrivacyFilter handles that elsewhere).
//   - Block the EventEngine worker thread.

#pragma once

#include "../Events/IEventListener.h"
#include "../Context/ContextEngine.h"
#include "../Focus/FocusManager.h"
#include "../Announcement/AnnouncementEngine.h"
#include "../Speech/SpeechManager.h"
#include "../Speech/SpeechPolicy.h"
#include "../Audio/EarconManager.h"
#include "../Browse/VirtualCursor.h"
#include "../Browse/VirtualDocument.h"
#include "../Table/TableNavigator.h"
#include "../Adapters/AdapterRegistry.h"
#include "SayAll.h"
#include "ClipboardReader.h"

#include <atomic>
#include <mutex>
#include <memory>
#include <thread>
#include <string>

namespace AccessOS {

class AccessReader : public IEventListener {
public:
    /// Construct with non-owning pointers to all required subsystems.
    /// All pointers must remain valid for the lifetime of this object.
    /// earconManager is optional (may be nullptr).
    explicit AccessReader(ContextEngine*       contextEngine,
                          FocusManager*        focusManager,
                          SpeechManager*       speechManager,
                          Audio::EarconManager* earconManager = nullptr);

    ~AccessReader() override = default;

    // Not copyable or movable.
    AccessReader(const AccessReader&)            = delete;
    AccessReader& operator=(const AccessReader&) = delete;

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /// Enable event routing. Call once after all subsystems are ready.
    void Initialize() noexcept;

    /// Disable event routing. Safe to call multiple times.
    void Shutdown() noexcept;

    bool IsRunning() const noexcept { return m_running.load(); }

    // ── IEventListener ────────────────────────────────────────────────────────

    /// Receives normalized AccessEvents from EventEngine.
    /// Routes each event through the pipeline.
    void OnEvent(const AccessEvent& event) override;

    // ── Control API ──────────────────────────────────────────────────────────

    /// Replace the active speech policy.
    void SetPolicy(SpeechPolicy policy);

    /// Get the current speech policy.
    SpeechPolicy GetPolicy() const;

    /// Stop all speech immediately.
    void StopSpeech();

    /// Read the currently focused element again (re-announce).
    void ReadFocused();

    /// Returns the window title that was current on the last focus event.
    std::string LastWindowTitle() const;

    // ── Typing Echo (ACCESSOS-032) ────────────────────────────────────────────

    enum class TypingEchoMode : uint8_t {
        Off     = 0,
        Char    = 1,  // speak each character typed
        Word    = 2,  // speak each completed word
        Both    = 3,  // speak char + completed word
    };

    void SetTypingEchoMode(TypingEchoMode mode);
    TypingEchoMode GetTypingEchoMode() const;

    // ── Clipboard Reading (ACCESSOS-033) ──────────────────────────────────────

    /// Read clipboard text and speak it. Returns false if clipboard is empty.
    bool ReadClipboard();

    // ── Browse Mode ───────────────────────────────────────────────────────────

    /// Toggle browse mode on/off. Returns true if now active.
    bool ToggleBrowseMode();
    bool IsBrowseMode() const noexcept { return m_browseMode.load(); }

    /// Move the virtual cursor forward (MoveNextElement). Speaks result.
    void BrowseMoveNext();
    /// Move the virtual cursor backward. Speaks result.
    void BrowseMovePrev();
    /// Move to next heading (any level). Speaks result.
    void BrowseMoveNextHeading();
    /// Move to prev heading. Speaks result.
    void BrowseMovePrevHeading();

    // ── Say All ───────────────────────────────────────────────────────────────

    /// Start continuous reading from current browse cursor position.
    /// Launches a background thread; returns immediately.
    void StartSayAll();

    /// Interrupt Say All.
    void StopSayAll();

    bool IsSayAllActive() const noexcept;

    // ── Table reading ─────────────────────────────────────────────────────────

    /// True if a table is currently active (last focused element was table/cell)
    bool HasActiveTable() const;
    /// Announce the current table cell (col + row header + text + position)
    std::string GetCurrentTableCell() const;
    /// Move table cursor and return announcement text ("" = boundary)
    std::string TableMoveNext();
    std::string TableMovePrev();
    std::string TableMoveNextRow();
    std::string TableMovePrevRow();

private:
    // ── Event handlers (called from OnEvent, on the worker thread) ────────────

    void HandleFocusChanged(const AccessEvent& event);
    void HandlePropertyChanged(const AccessEvent& event);
    void HandleLiveRegion(const AccessEvent& event);

    /// Submit an Announcement to SpeechManager.
    void Speak(const Announcement& ann);

    // ── State ─────────────────────────────────────────────────────────────────
    ContextEngine*        m_contextEngine;   // non-owning
    FocusManager*         m_focusManager;    // non-owning
    SpeechManager*        m_speechManager;   // non-owning
    Audio::EarconManager* m_earconManager;   // non-owning, may be nullptr

    /// Fire an earcon (no-op if m_earconManager is null or earcons disabled)
    void PlayEarcon(Audio::EarconId id);

    // Browse mode state
    std::atomic<bool>                    m_browseMode{ false };
    std::shared_ptr<Browse::VirtualDocument> m_browseDoc;
    Browse::VirtualCursor                m_cursor;

    // Say All
    SayAll               m_sayAll;
    std::thread          m_sayAllThread;
    mutable std::mutex   m_sayAllMutex;

    // Table reading state
    Table::TableNavigator                m_tableNav;

    std::atomic<bool>   m_running{ false };

    mutable std::mutex  m_policyMutex;
    SpeechPolicy        m_policy;

    mutable std::mutex  m_titleMutex;
    std::string         m_lastWindowTitle;

    // Typing echo
    std::atomic<uint8_t> m_typingEcho{ 0 }; // TypingEchoMode
    std::string          m_lastTypedText;
    mutable std::mutex   m_typingMutex;

    // Handler for TextChanged events (typing echo)
    void HandleTextChanged(const AccessEvent& event);

    // Helper: speak a plain string at Normal priority
    void SpeakRaw(const std::string& text);
};

} // namespace AccessOS
