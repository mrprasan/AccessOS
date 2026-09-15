// AccessOS/src/Core/Navigation/NavigationEngine.h
//
// NavigationEngine — provides semantic navigation over the AccessNode graph.
//
// Why: Navigation must be independent of speech, keyboard, and provider.
//      It operates only on the SemanticModel and returns AccessNode results.
//      Callers (SpeechManager, FocusManager) decide what to do with results.
//
// Navigation tiers (Section 11):
//   Basic:    next, previous, parent, first child, last child, focused
//   Semantic: heading, link, button, form field, checkbox, radio, list item
//   Advanced: table, landmark, document (deferred to ACCESSOS-008+)
//
// Must NOT:
//   - Speak, log user content, or call SAPI
//   - Hold live COM/UIA pointers
//   - Depend on any specific provider
//
// Threading: All methods are thread-safe (reads only from SemanticModel).

#pragma once

#include "../Semantic/AccessNode.h"
#include "../Semantic/SemanticModel.h"
#include "../Error/AccessError.h"

#include <optional>
#include <vector>

namespace AccessOS {

// Navigation direction for linear (next/previous) traversal.
enum class NavigationDirection {
    Next,
    Previous,
};

class NavigationEngine {
public:
    explicit NavigationEngine(SemanticModel* model);

    // ── Basic navigation ──────────────────────────────────────────────────────

    // Returns the currently focused element.
    std::optional<AccessNode> GetFocused() const;

    // Returns the next/previous sibling of the focused element.
    std::optional<AccessNode> Navigate(NavigationDirection direction) const;

    // Returns the parent of the focused element.
    std::optional<AccessNode> GetParent() const;

    // Returns the first child of the focused element.
    std::optional<AccessNode> GetFirstChild() const;

    // Returns the last child of the focused element.
    std::optional<AccessNode> GetLastChild() const;

    // ── Semantic navigation ────────────────────────────────────────────────────
    // All find methods search the entire cache for the next/previous
    // element of the given role, relative to the currently focused element.

    std::optional<AccessNode> FindNext(AccessRole role,
                                       NavigationDirection dir =
                                           NavigationDirection::Next) const;

    // Convenience semantic navigation methods.
    std::optional<AccessNode> NextHeading()    const;
    std::optional<AccessNode> PrevHeading()    const;
    std::optional<AccessNode> NextLink()       const;
    std::optional<AccessNode> PrevLink()       const;
    std::optional<AccessNode> NextButton()     const;
    std::optional<AccessNode> PrevButton()     const;
    std::optional<AccessNode> NextFormField()  const;
    std::optional<AccessNode> PrevFormField()  const;

    // ── All elements of a role ─────────────────────────────────────────────────
    std::vector<AccessNode> AllOfRole(AccessRole role) const;

private:
    // Returns all cached nodes sorted by their ID (stable traversal order).
    std::vector<AccessNode> SortedNodes() const;

    // Returns the index of the focused element in the sorted list.
    // Returns -1 if not found.
    int FocusedIndex(const std::vector<AccessNode>& sorted) const;

    SemanticModel* m_model;  // Non-owning — must outlive NavigationEngine.
};

} // namespace AccessOS
