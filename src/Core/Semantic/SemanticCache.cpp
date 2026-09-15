// AccessOS/src/Core/Semantic/SemanticCache.cpp

#include "SemanticCache.h"

namespace AccessOS {

void SemanticCache::Update(AccessNode node) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    const uint64_t id = node.id;
    m_nodes[id] = std::move(node);
}

void SemanticCache::Remove(uint64_t id) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_nodes.erase(id);
    if (m_focusedId == id) {
        m_focusedId = 0;
    }
}

void SemanticCache::Clear() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_nodes.clear();
    m_focusedId = 0;
}

std::optional<AccessNode> SemanticCache::Get(uint64_t id) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_nodes.find(id);
    if (it == m_nodes.end()) return std::nullopt;
    return it->second;
}

std::optional<AccessNode> SemanticCache::GetFocused() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    if (m_focusedId == 0) return std::nullopt;
    auto it = m_nodes.find(m_focusedId);
    if (it == m_nodes.end()) return std::nullopt;
    return it->second;
}

void SemanticCache::SetFocusedId(uint64_t id) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_focusedId = id;
}

std::vector<AccessNode> SemanticCache::GetChildren(uint64_t parentId) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    std::vector<AccessNode> children;
    for (const auto& [id, node] : m_nodes) {
        if (node.parentId == parentId) {
            children.push_back(node);
        }
    }
    return children;
}

size_t SemanticCache::Size() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_nodes.size();
}

bool SemanticCache::Contains(uint64_t id) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_nodes.count(id) > 0;
}

std::vector<AccessNode> SemanticCache::FindAll(
    std::function<bool(const AccessNode&)> predicate) const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    std::vector<AccessNode> results;
    for (const auto& [id, node] : m_nodes) {
        if (predicate(node)) {
            results.push_back(node);
        }
    }
    return results;
}

} // namespace AccessOS
