#pragma once
// VirtualDocument.h — Flat linearised accessible document (ACCESSOS-026)
//
// Built from an ordered list of VirtualNodes provided by the caller.
// The VirtualCursor operates on this document.

#include "VirtualNode.h"
#include <vector>
#include <string>
#include <optional>

namespace AccessOS {
namespace Browse {

class VirtualDocument {
public:
    VirtualDocument() = default;

    // Build document from an ordered node list (assigns nodeIndex)
    explicit VirtualDocument(std::vector<VirtualNode> nodes);

    // Add a node (appended, nodeIndex assigned automatically)
    void AddNode(VirtualNode node);

    // Replace all nodes
    void SetNodes(std::vector<VirtualNode> nodes);

    // Clear all nodes
    void Clear();

    size_t NodeCount() const noexcept { return m_nodes.size(); }
    bool   IsEmpty()   const noexcept { return m_nodes.empty(); }

    // Access by index; throws std::out_of_range if invalid
    const VirtualNode& NodeAt(size_t index) const;

    // Find first node at or after `fromIndex` matching predicate
    std::optional<size_t> FindNext(size_t fromIndex,
        bool (*predicate)(const VirtualNode&)) const;

    // Find last node at or before `fromIndex` matching predicate
    std::optional<size_t> FindPrev(size_t fromIndex,
        bool (*predicate)(const VirtualNode&)) const;

    // Full document text (all node texts joined with spaces)
    std::string FullText() const;

    const std::vector<VirtualNode>& Nodes() const noexcept { return m_nodes; }

private:
    std::vector<VirtualNode> m_nodes;
};

} // namespace Browse
} // namespace AccessOS
