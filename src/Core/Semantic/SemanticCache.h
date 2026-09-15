// AccessOS/src/Core/Semantic/SemanticCache.h
//
// In-memory cache of AccessNode snapshots keyed by element ID.
//
// Why: Navigation, speech, and inspection all need to look up elements
//      by ID without re-querying the provider. The cache is the single
//      source of truth for the current accessibility tree state.
//
// Responsibilities:
//   - Store and retrieve AccessNode snapshots by ID
//   - Update nodes when events arrive (focus, property, structure changes)
//   - Invalidate stale nodes on structure changes
//   - Provide the current focused node
//   - Provide root and children lookups
//
// Must NOT:
//   - Hold live COM/UIA pointers
//   - Perform speech or navigation decisions
//   - Block the event thread
//
// Threading:
//   - All public methods are thread-safe via internal read-write mutex.
//   - Updates arrive from the EventEngine worker thread.
//   - Reads come from navigation/speech/inspection threads.

#pragma once

#include "AccessNode.h"
#include "../Error/AccessError.h"

#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include <vector>
#include <functional>

namespace AccessOS {

class SemanticCache {
public:
    SemanticCache()  = default;
    ~SemanticCache() = default;

    // Insert or replace a node. Thread-safe.
    void Update(AccessNode node);

    // Remove a node by ID. Thread-safe.
    void Remove(uint64_t id);

    // Clear all cached nodes. Thread-safe.
    void Clear();

    // Retrieve a node by ID. Returns nullopt if not found. Thread-safe.
    std::optional<AccessNode> Get(uint64_t id) const;

    // Returns the currently focused node, or nullopt if none. Thread-safe.
    std::optional<AccessNode> GetFocused() const;

    // Set the focused node ID. Thread-safe.
    void SetFocusedId(uint64_t id);

    // Returns all children of the given parent ID. Thread-safe.
    std::vector<AccessNode> GetChildren(uint64_t parentId) const;

    // Returns the total number of cached nodes. Thread-safe.
    size_t Size() const;

    // Returns true if a node with the given ID exists. Thread-safe.
    bool Contains(uint64_t id) const;

    // Iterate all nodes with a predicate. Returns matching nodes. Thread-safe.
    std::vector<AccessNode> FindAll(
        std::function<bool(const AccessNode&)> predicate) const;

private:
    mutable std::shared_mutex               m_mutex;
    std::unordered_map<uint64_t, AccessNode> m_nodes;
    uint64_t                                m_focusedId{ 0 };
};

} // namespace AccessOS
