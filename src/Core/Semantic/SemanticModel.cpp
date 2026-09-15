// AccessOS/src/Core/Semantic/SemanticModel.cpp

#include "SemanticModel.h"
#include "../Logging/Logger.h"

namespace AccessOS {

static constexpr const char* kComponent = "SemanticModel";

SemanticModel::SemanticModel() = default;

// ─── IEventListener ──────────────────────────────────────────────────────────

void SemanticModel::OnEvent(const AccessEvent& event) {
    switch (event.type) {
    case AccessEventType::FocusChanged:
        HandleFocusChanged(event);
        break;

    case AccessEventType::NameChanged:
    case AccessEventType::ValueChanged:
    case AccessEventType::StateChanged:
    case AccessEventType::DescriptionChanged:
        HandlePropertyChanged(event);
        break;

    case AccessEventType::StructureChanged:
        HandleStructureChanged(event);
        break;

    default:
        // Other event types are not yet consumed by the semantic model.
        break;
    }
}

// ─── Public API ──────────────────────────────────────────────────────────────

std::optional<AccessNode> SemanticModel::GetFocused() const {
    return m_cache.GetFocused();
}

std::optional<AccessNode> SemanticModel::GetNode(uint64_t id) const {
    return m_cache.Get(id);
}

std::vector<AccessNode> SemanticModel::FindAll(
    std::function<bool(const AccessNode&)> predicate) const
{
    return m_cache.FindAll(std::move(predicate));
}

void SemanticModel::AddFocusObserver(FocusObserver observer) {
    std::lock_guard<std::mutex> lock(m_observerMutex);
    m_focusObservers.push_back(std::move(observer));
}

void SemanticModel::Clear() {
    m_cache.Clear();
    ACOS_LOG_INFO(kComponent, "Semantic cache cleared");
}

size_t SemanticModel::CacheSize() const {
    return m_cache.Size();
}

// ─── Private event handlers ───────────────────────────────────────────────────

void SemanticModel::HandleFocusChanged(const AccessEvent& event) {
    AccessNode normalized = SemanticNormalizer::Normalize(event.element);

    if (normalized.id == 0) {
        ACOS_LOG_WARNING(kComponent, "FocusChanged event has zero element ID — ignoring");
        return;
    }

    // Mark focused state explicitly — event snapshot may not have set it.
    normalized.state = normalized.state | AccessState::Focused;

    m_cache.Update(normalized);
    m_cache.SetFocusedId(normalized.id);

    ACOS_LOG_DEBUG(kComponent,
        "Focus → [" + normalized.name + "] role=" +
        std::to_string(static_cast<int>(normalized.role)));

    // Notify observers — snapshot the list first to avoid holding lock.
    std::vector<FocusObserver> observers;
    {
        std::lock_guard<std::mutex> lock(m_observerMutex);
        observers = m_focusObservers;
    }
    for (const auto& obs : observers) {
        obs(normalized);
    }
}

void SemanticModel::HandlePropertyChanged(const AccessEvent& event) {
    if (event.element.id == 0) return;

    // If the node is already cached, merge the update.
    // If not cached yet, store the full snapshot.
    AccessNode normalized = SemanticNormalizer::Normalize(event.element);
    m_cache.Update(normalized);

    ACOS_LOG_DEBUG(kComponent,
        "Property changed for [" + normalized.name + "]");
}

void SemanticModel::HandleStructureChanged(const AccessEvent& event) {
    // On structure changes, update the affected node.
    // Full subtree invalidation is deferred to ACCESSOS-005 (FocusManager).
    if (event.element.id != 0) {
        AccessNode normalized = SemanticNormalizer::Normalize(event.element);
        m_cache.Update(normalized);
    }

    ACOS_LOG_DEBUG(kComponent, "Structure changed — cache updated");
}

} // namespace AccessOS
