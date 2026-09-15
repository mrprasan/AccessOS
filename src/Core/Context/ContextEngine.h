// AccessOS/src/Core/Context/ContextEngine.h
//
// ContextEngine — manages the current AppContext and notifies observers when it changes.
//
// Why: Multiple subsystems (speech, navigation, Braille) need to know the active
//      application context without each doing their own detection.  ContextEngine
//      is the single source of truth.  It owns a ContextDetector, holds the last
//      known context, and fires registered callbacks when the context changes.
//
// Lifecycle:
//   1. Construct once at startup.
//   2. Call Update(node) on every focus-change event.
//   3. Subscribers register via AddObserver().  They are called synchronously on
//      the caller's thread (typically the EventEngine worker thread).
//
// Threading: Update() and AddObserver() must be called from a single thread
//            (the EventEngine worker thread).  GetCurrent() may be called from
//            any thread — it returns a copy under a shared_mutex.

#pragma once

#include "AppContext.h"
#include "ContextDetector.h"
#include "../Semantic/AccessNode.h"

#include <functional>
#include <shared_mutex>
#include <vector>

namespace AccessOS {

class ContextEngine {
public:
    ContextEngine();
    ~ContextEngine() = default;

    // Not copyable or movable — owns state and observer list.
    ContextEngine(const ContextEngine&)            = delete;
    ContextEngine& operator=(const ContextEngine&) = delete;

    /// Process a focus-change node.  If the resulting AppContext differs from
    /// the current one, observers are fired synchronously and the stored context
    /// is updated.
    /// Returns true when the context changed.
    bool Update(const AccessNode& node) noexcept;

    /// Retrieve the current context snapshot (thread-safe copy).
    AppContext GetCurrent() const noexcept;

    /// Register a callback that fires whenever the context changes.
    /// The callback receives the new AppContext by const-ref.
    /// Callbacks must not call Update() or AddObserver() (would deadlock).
    void AddObserver(ContextObserver observer);

    /// Remove all registered observers.
    void ClearObservers() noexcept;

    /// Returns the number of registered observers.
    std::size_t ObserverCount() const noexcept;

private:
    /// Returns true when two contexts are functionally different (warrant notification).
    static bool IsDifferent(const AppContext& a, const AppContext& b) noexcept;

    mutable std::shared_mutex   m_mutex;
    AppContext                  m_current;
    std::vector<ContextObserver> m_observers;
};

} // namespace AccessOS
