// AccessOS/src/Core/Navigation/NavigationEngine.cpp

#include "NavigationEngine.h"
#include "../Logging/Logger.h"

#include <algorithm>

namespace AccessOS {

static constexpr const char* kComponent = "NavigationEngine";

NavigationEngine::NavigationEngine(SemanticModel* model)
    : m_model(model)
{
}

// ── Basic navigation ──────────────────────────────────────────────────────────

std::optional<AccessNode> NavigationEngine::GetFocused() const {
    if (!m_model) return std::nullopt;
    return m_model->GetFocused();
}

std::optional<AccessNode> NavigationEngine::Navigate(
    NavigationDirection direction) const
{
    if (!m_model) return std::nullopt;

    const auto sorted = SortedNodes();
    if (sorted.empty()) return std::nullopt;

    const int idx = FocusedIndex(sorted);
    if (idx < 0) return std::nullopt;

    if (direction == NavigationDirection::Next) {
        if (idx + 1 < static_cast<int>(sorted.size())) {
            return sorted[static_cast<size_t>(idx + 1)];
        }
    } else {
        if (idx - 1 >= 0) {
            return sorted[static_cast<size_t>(idx - 1)];
        }
    }
    return std::nullopt;
}

std::optional<AccessNode> NavigationEngine::GetParent() const {
    if (!m_model) return std::nullopt;
    auto focused = m_model->GetFocused();
    if (!focused.has_value() || focused->parentId == 0) return std::nullopt;
    return m_model->GetNode(focused->parentId);
}

std::optional<AccessNode> NavigationEngine::GetFirstChild() const {
    if (!m_model) return std::nullopt;
    auto focused = m_model->GetFocused();
    if (!focused.has_value() || focused->childIds.empty()) return std::nullopt;
    return m_model->GetNode(focused->childIds.front());
}

std::optional<AccessNode> NavigationEngine::GetLastChild() const {
    if (!m_model) return std::nullopt;
    auto focused = m_model->GetFocused();
    if (!focused.has_value() || focused->childIds.empty()) return std::nullopt;
    return m_model->GetNode(focused->childIds.back());
}

// ── Semantic navigation ───────────────────────────────────────────────────────

std::optional<AccessNode> NavigationEngine::FindNext(
    AccessRole role, NavigationDirection dir) const
{
    if (!m_model) return std::nullopt;

    const auto sorted = SortedNodes();
    if (sorted.empty()) return std::nullopt;

    const int idx = FocusedIndex(sorted);
    const int n   = static_cast<int>(sorted.size());

    if (dir == NavigationDirection::Next) {
        for (int i = idx + 1; i < n; ++i) {
            if (sorted[static_cast<size_t>(i)].role == role) {
                return sorted[static_cast<size_t>(i)];
            }
        }
        // Wrap around from beginning.
        for (int i = 0; i < idx; ++i) {
            if (sorted[static_cast<size_t>(i)].role == role) {
                return sorted[static_cast<size_t>(i)];
            }
        }
    } else {
        for (int i = idx - 1; i >= 0; --i) {
            if (sorted[static_cast<size_t>(i)].role == role) {
                return sorted[static_cast<size_t>(i)];
            }
        }
        // Wrap around from end.
        for (int i = n - 1; i > idx; --i) {
            if (sorted[static_cast<size_t>(i)].role == role) {
                return sorted[static_cast<size_t>(i)];
            }
        }
    }
    return std::nullopt;
}

std::optional<AccessNode> NavigationEngine::NextHeading()   const { return FindNext(AccessRole::Heading,    NavigationDirection::Next); }
std::optional<AccessNode> NavigationEngine::PrevHeading()   const { return FindNext(AccessRole::Heading,    NavigationDirection::Previous); }
std::optional<AccessNode> NavigationEngine::NextLink()      const { return FindNext(AccessRole::Link,       NavigationDirection::Next); }
std::optional<AccessNode> NavigationEngine::PrevLink()      const { return FindNext(AccessRole::Link,       NavigationDirection::Previous); }
std::optional<AccessNode> NavigationEngine::NextButton()    const { return FindNext(AccessRole::Button,     NavigationDirection::Next); }
std::optional<AccessNode> NavigationEngine::PrevButton()    const { return FindNext(AccessRole::Button,     NavigationDirection::Previous); }
std::optional<AccessNode> NavigationEngine::NextFormField() const { return FindNext(AccessRole::Edit,       NavigationDirection::Next); }
std::optional<AccessNode> NavigationEngine::PrevFormField() const { return FindNext(AccessRole::Edit,       NavigationDirection::Previous); }

std::vector<AccessNode> NavigationEngine::AllOfRole(AccessRole role) const {
    if (!m_model) return {};
    return m_model->FindAll([role](const AccessNode& n) {
        return n.role == role;
    });
}

// ── Private helpers ───────────────────────────────────────────────────────────

std::vector<AccessNode> NavigationEngine::SortedNodes() const {
    auto nodes = m_model->FindAll([](const AccessNode&) { return true; });
    // Sort by ID for deterministic traversal order.
    std::sort(nodes.begin(), nodes.end(),
              [](const AccessNode& a, const AccessNode& b) {
                  return a.id < b.id;
              });
    return nodes;
}

int NavigationEngine::FocusedIndex(const std::vector<AccessNode>& sorted) const {
    auto focused = m_model->GetFocused();
    if (!focused.has_value()) return -1;
    const uint64_t id = focused->id;
    for (int i = 0; i < static_cast<int>(sorted.size()); ++i) {
        if (sorted[static_cast<size_t>(i)].id == id) return i;
    }
    return -1;
}

} // namespace AccessOS
