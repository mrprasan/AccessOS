// AccessOS/src/Core/Focus/FocusManager.cpp

#include "FocusManager.h"
#include "../Logging/Logger.h"

namespace AccessOS {

static constexpr const char* kComponent = "FocusManager";

// ─── Constructors ─────────────────────────────────────────────────────────────

FocusManager::FocusManager() = default;

FocusManager::FocusManager(SemanticModel* model) {
    Initialize(model);
}

// ─── Initialize ──────────────────────────────────────────────────────────────

void FocusManager::Initialize(SemanticModel* model) {
    if (!model) {
        ACOS_LOG_ERROR(kComponent, "Initialize called with null SemanticModel");
        return;
    }
    m_model = model;

    // Register as a focus observer on the SemanticModel.
    // The lambda captures 'this' — FocusManager must outlive SemanticModel.
    m_model->AddFocusObserver([this](const AccessNode& node) {
        OnFocusChanged(node);
    });

    ACOS_LOG_INFO(kComponent, "FocusManager initialized");
}

// ─── Public API ──────────────────────────────────────────────────────────────

std::optional<AccessNode> FocusManager::GetCurrentFocus() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentFocus;
}

std::optional<AccessNode> FocusManager::GetPreviousFocus() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    // History is newest-first; index 0 = current (just pushed), index 1 = previous.
    if (m_history.size() < 2) return std::nullopt;
    return m_history[1];
}

std::vector<AccessNode> FocusManager::GetHistory(size_t maxCount) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<AccessNode> result;
    const size_t count = std::min(maxCount, m_history.size());
    result.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        result.push_back(m_history[i]);
    }
    return result;
}

void FocusManager::AddObserver(FocusChangeObserver observer) {
    std::lock_guard<std::mutex> lock(m_observerMutex);
    m_observers.push_back(std::move(observer));
}

uint64_t FocusManager::DuplicatesSuppressed() const noexcept {
    return m_duplicateCount.load();
}

uint64_t FocusManager::FocusChangeCount() const noexcept {
    return m_changeCount.load();
}

// ─── Private ─────────────────────────────────────────────────────────────────

void FocusManager::OnFocusChanged(const AccessNode& node) {
    // Fast path: check if this is a genuine change before taking the lock.
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!IsGenuineChange(node)) {
            ++m_duplicateCount;
            ACOS_LOG_DEBUG(kComponent,
                "Duplicate focus event suppressed for [" + node.name + "]");
            return;
        }

        m_currentFocus = node;
        PushHistory(node);
        ++m_changeCount;
    }

    ACOS_LOG_INFO(kComponent,
        "Focus → [" + node.name + "] role=" +
        std::to_string(static_cast<int>(node.role)) +
        " pid=" + std::to_string(node.processId));

    NotifyObservers(node);
}

bool FocusManager::IsGenuineChange(const AccessNode& incoming) const {
    // Must be called with m_mutex held.

    if (!m_currentFocus.has_value()) return true;   // First focus event

    const AccessNode& current = m_currentFocus.value();

    // Same element ID — duplicate event.
    if (incoming.id != 0 && incoming.id == current.id) return false;

    // Same name + same role + same process — likely a duplicate
    // from window activation without actual focus movement.
    // Only suppress if IDs are both valid (non-zero).
    if (incoming.id == 0 && current.id == 0 &&
        incoming.name == current.name &&
        incoming.role == current.role &&
        incoming.processId == current.processId)
    {
        return false;
    }

    return true;
}

void FocusManager::PushHistory(const AccessNode& node) {
    // Must be called with m_mutex held.
    m_history.push_front(node);
    if (m_history.size() > kFocusHistoryMaxSize) {
        m_history.pop_back();
    }
}

void FocusManager::NotifyObservers(const AccessNode& node) {
    // Snapshot observer list without holding observer lock during dispatch.
    std::vector<FocusChangeObserver> observers;
    {
        std::lock_guard<std::mutex> lock(m_observerMutex);
        observers = m_observers;
    }
    for (const auto& obs : observers) {
        obs(node);
    }
}

} // namespace AccessOS
