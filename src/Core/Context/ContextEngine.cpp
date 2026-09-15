// AccessOS/src/Core/Context/ContextEngine.cpp

#include "ContextEngine.h"

namespace AccessOS {

// ── Constructor ───────────────────────────────────────────────────────────────

ContextEngine::ContextEngine()
    : m_current{}
{
}

// ── Update ────────────────────────────────────────────────────────────────────

bool ContextEngine::Update(const AccessNode& node) noexcept {
    AppContext next = ContextDetector::Detect(node);

    // Check if the context actually changed before acquiring the write lock.
    {
        std::shared_lock<std::shared_mutex> rlock(m_mutex);
        if (!IsDifferent(m_current, next)) return false;
    }

    // Promote to write lock and update.
    std::vector<ContextObserver> observers;
    {
        std::unique_lock<std::shared_mutex> wlock(m_mutex);
        // Re-check under write lock (another thread could have updated).
        if (!IsDifferent(m_current, next)) return false;

        m_current = next;
        observers = m_observers;   // copy so we can fire without holding lock
    }

    // Fire observers outside the lock.
    for (const auto& obs : observers) {
        obs(next);
    }

    return true;
}

// ── GetCurrent ────────────────────────────────────────────────────────────────

AppContext ContextEngine::GetCurrent() const noexcept {
    std::shared_lock<std::shared_mutex> rlock(m_mutex);
    return m_current;
}

// ── Observer management ───────────────────────────────────────────────────────

void ContextEngine::AddObserver(ContextObserver observer) {
    std::unique_lock<std::shared_mutex> wlock(m_mutex);
    m_observers.push_back(std::move(observer));
}

void ContextEngine::ClearObservers() noexcept {
    std::unique_lock<std::shared_mutex> wlock(m_mutex);
    m_observers.clear();
}

std::size_t ContextEngine::ObserverCount() const noexcept {
    std::shared_lock<std::shared_mutex> rlock(m_mutex);
    return m_observers.size();
}

// ── IsDifferent ───────────────────────────────────────────────────────────────

bool ContextEngine::IsDifferent(const AppContext& a, const AppContext& b) noexcept {
    return a.type        != b.type        ||
           a.processId   != b.processId   ||
           a.processName != b.processName ||
           a.isModal     != b.isModal     ||
           a.isWebContent!= b.isWebContent;
}

} // namespace AccessOS
