// AccessOS/src/Core/Semantic/SemanticModel.h
//
// Top-level semantic model — receives events from the EventEngine,
// normalizes incoming AccessNode snapshots, and updates the SemanticCache.
//
// Why: This is the single point where raw provider events become
//      canonical AccessOS semantic state. Navigation, speech, and
//      inspection all read from SemanticModel, never from providers.
//
// Responsibilities:
//   - Implement IEventListener to receive events from EventEngine
//   - Normalize incoming snapshots via SemanticNormalizer
//   - Update SemanticCache with normalized nodes
//   - Maintain focus state in the cache
//   - Notify registered observers when the focused node changes
//
// Must NOT:
//   - Perform speech, navigation, or UI operations
//   - Hold live COM/UIA pointers
//   - Block the event thread beyond cache update
//
// Threading:
//   - OnEvent() is called on the EventEngine worker thread.
//   - GetFocused() / GetNode() are called from navigation/speech threads.
//   - SemanticCache handles internal synchronization.

#pragma once

#include "SemanticCache.h"
#include "SemanticNormalizer.h"
#include "../Events/IEventListener.h"
#include "../Error/AccessError.h"

#include <functional>
#include <mutex>
#include <vector>
#include <memory>

namespace AccessOS {

// Observer callback — invoked when the focused element changes.
// Called on the EventEngine worker thread. Must not block.
using FocusObserver = std::function<void(const AccessNode&)>;

class SemanticModel final : public IEventListener {
public:
    SemanticModel();
    ~SemanticModel() override = default;

    // IEventListener — receives normalized events from EventEngine.
    void OnEvent(const AccessEvent& event) override;

    // Returns the currently focused AccessNode, or nullopt if none.
    std::optional<AccessNode> GetFocused() const;

    // Returns a node by ID, or nullopt if not found.
    std::optional<AccessNode> GetNode(uint64_t id) const;

    // Returns all nodes matching a predicate.
    std::vector<AccessNode> FindAll(
        std::function<bool(const AccessNode&)> predicate) const;

    // Register an observer for focus changes.
    // Observer is called on the EventEngine worker thread.
    void AddFocusObserver(FocusObserver observer);

    // Clear all cached nodes (e.g. on application switch).
    void Clear();

    // Returns the number of cached nodes — for diagnostics.
    size_t CacheSize() const;

private:
    void HandleFocusChanged(const AccessEvent& event);
    void HandlePropertyChanged(const AccessEvent& event);
    void HandleStructureChanged(const AccessEvent& event);

    SemanticCache               m_cache;

    mutable std::mutex          m_observerMutex;
    std::vector<FocusObserver>  m_focusObservers;
};

} // namespace AccessOS
