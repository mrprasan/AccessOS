// AccessOS/src/Core/Focus/FocusManager.h
//
// FocusManager — tracks the currently focused accessible element,
// detects focus changes, maintains focus history, and prevents
// duplicate announcements.
//
// Why: Focus is the primary navigation signal for a screen reader.
//      Raw UIA focus events can fire repeatedly for the same element
//      (e.g. during window activation). The FocusManager filters
//      duplicates, validates elements, and provides a stable focused
//      element to the presentation layer.
//
// Responsibilities:
//   - Receive focus change notifications from SemanticModel
//   - Detect genuine focus changes (not duplicate/stale events)
//   - Maintain focus history (last N elements)
//   - Handle application and window transitions
//   - Notify registered observers only on genuine changes
//   - Track focus latency for performance monitoring
//
// Must NOT:
//   - Perform speech or Braille output directly
//   - Hold live COM/UIA pointers
//   - Query UIA directly — reads only from SemanticModel
//
// Threading:
//   - OnFocusChanged() arrives on the SemanticModel observer thread
//     (EventEngine worker thread). Must not block.
//   - GetCurrentFocus() is safe to call from any thread.

#pragma once

#include "../Semantic/AccessNode.h"
#include "../Semantic/SemanticModel.h"
#include "../Error/AccessError.h"

#include <functional>
#include <mutex>
#include <deque>
#include <optional>
#include <chrono>
#include <atomic>

namespace AccessOS {

// Observer called when a genuine focus change is confirmed.
// Called on the EventEngine worker thread — must not block.
using FocusChangeObserver = std::function<void(const AccessNode& focused)>;

// Maximum number of elements retained in focus history.
static constexpr size_t kFocusHistoryMaxSize = 32;

class FocusManager {
public:
    FocusManager();
    explicit FocusManager(SemanticModel* model);
    ~FocusManager() = default;

    // Initialize — registers as a focus observer on the SemanticModel.
    // Must be called before any other method.
    void Initialize(SemanticModel* model);

    // Returns the current focused AccessNode, or nullopt if none.
    // Thread-safe.
    std::optional<AccessNode> GetCurrentFocus() const;

    // Returns the previous focused AccessNode, or nullopt if none.
    // Thread-safe.
    std::optional<AccessNode> GetPreviousFocus() const;

    // Returns up to N recent focus history entries (newest first).
    // Thread-safe.
    std::vector<AccessNode> GetHistory(size_t maxCount = 8) const;

    // Register an observer for genuine focus changes.
    // Called on the EventEngine worker thread.
    void AddObserver(FocusChangeObserver observer);

    // Returns the number of duplicate focus events suppressed.
    // For diagnostics and performance monitoring.
    uint64_t DuplicatesSuppressed() const noexcept;

    // Returns the number of genuine focus changes processed.
    uint64_t FocusChangeCount() const noexcept;

private:
    // Called by SemanticModel when any focus change occurs.
    // Decides whether it is a genuine change or a duplicate.
    void OnFocusChanged(const AccessNode& node);

    // Returns true if the incoming node represents a genuine focus change
    // (different element, not a duplicate, not stale).
    bool IsGenuineChange(const AccessNode& incoming) const;

    // Pushes a node into the history deque, maintaining max size.
    void PushHistory(const AccessNode& node);

    // Notifies all registered observers.
    void NotifyObservers(const AccessNode& node);

    SemanticModel*                  m_model{ nullptr };

    mutable std::mutex              m_mutex;
    std::optional<AccessNode>       m_currentFocus;
    std::deque<AccessNode>          m_history;      // newest at front

    mutable std::mutex              m_observerMutex;
    std::vector<FocusChangeObserver> m_observers;

    std::atomic<uint64_t>           m_duplicateCount{ 0 };
    std::atomic<uint64_t>           m_changeCount{ 0 };
};

} // namespace AccessOS
