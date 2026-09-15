// VirtualDocument.cpp — Flat linearised accessible document (ACCESSOS-026)
#include "VirtualDocument.h"
#include <stdexcept>

namespace AccessOS {
namespace Browse {

VirtualDocument::VirtualDocument(std::vector<VirtualNode> nodes) {
    SetNodes(std::move(nodes));
}

void VirtualDocument::AddNode(VirtualNode node) {
    node.nodeIndex = static_cast<uint32_t>(m_nodes.size());
    m_nodes.push_back(std::move(node));
}

void VirtualDocument::SetNodes(std::vector<VirtualNode> nodes) {
    m_nodes = std::move(nodes);
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_nodes.size()); ++i) {
        m_nodes[i].nodeIndex = i;
    }
}

void VirtualDocument::Clear() {
    m_nodes.clear();
}

const VirtualNode& VirtualDocument::NodeAt(size_t index) const {
    if (index >= m_nodes.size())
        throw std::out_of_range("VirtualDocument::NodeAt: index out of range");
    return m_nodes[index];
}

std::optional<size_t> VirtualDocument::FindNext(size_t fromIndex,
    bool (*predicate)(const VirtualNode&)) const {
    for (size_t i = fromIndex; i < m_nodes.size(); ++i) {
        if (predicate(m_nodes[i])) return i;
    }
    return std::nullopt;
}

std::optional<size_t> VirtualDocument::FindPrev(size_t fromIndex,
    bool (*predicate)(const VirtualNode&)) const {
    if (m_nodes.empty()) return std::nullopt;
    size_t i = std::min(fromIndex, m_nodes.size() - 1);
    while (true) {
        if (predicate(m_nodes[i])) return i;
        if (i == 0) break;
        --i;
    }
    return std::nullopt;
}

std::string VirtualDocument::FullText() const {
    std::string out;
    for (const auto& n : m_nodes) {
        if (!out.empty() && !n.text.empty()) out += ' ';
        out += n.text;
    }
    return out;
}

} // namespace Browse
} // namespace AccessOS
