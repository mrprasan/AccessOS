// AccessOS/src/Core/Semantic/SemanticNormalizer.cpp

#include "SemanticNormalizer.h"

#include <algorithm>
#include <cctype>

namespace AccessOS {

AccessNode SemanticNormalizer::Normalize(AccessNode node) {
    NormalizeName(node);
    NormalizeRole(node);
    DeriveMultiLineState(node);
    return node;
}

// ─── Private helpers ──────────────────────────────────────────────────────────

std::string SemanticNormalizer::Trim(std::string s) {
    // Trim leading whitespace
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) {
        return !std::isspace(c);
    }));
    // Trim trailing whitespace
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) {
        return !std::isspace(c);
    }).base(), s.end());
    return s;
}

void SemanticNormalizer::NormalizeName(AccessNode& node) {
    node.name        = Trim(node.name);
    node.description = Trim(node.description);
    node.helpText    = Trim(node.helpText);
    node.value       = Trim(node.value);

    // If name is empty, try value as a fallback for static text roles.
    // This is a common UIA behavior for labels that expose text as value.
    if (node.name.empty() &&
        node.role == AccessRole::StaticText &&
        !node.value.empty())
    {
        node.name = node.value;
    }
}

void SemanticNormalizer::NormalizeRole(AccessNode& node) {
    // UIA Edit control with multi-line flag not set yet — see DeriveMultiLineState.
    // Nothing to change here for now; placeholder for future MSAA/IA2 role mapping.
    (void)node;
}

void SemanticNormalizer::DeriveMultiLineState(AccessNode& node) {
    // UIA does not always expose the MultiLine property directly.
    // For Edit controls, we set MultiLine as a default because most text
    // areas in applications are multi-line. Single-line overrides come
    // from the ValuePattern is-read-only or ControlType-specific knowledge.
    //
    // KNOWN LIMITATION: Accurate single-vs-multi-line detection requires
    // querying the TextPattern or LegacyIAccessible state — not yet implemented.
    if (node.role == AccessRole::Edit ||
        node.role == AccessRole::MultiLineEdit)
    {
        node.state = node.state | AccessState::MultiLine;
    }

    // Password fields must always be marked Protected — never log their content.
    if (node.role == AccessRole::PasswordEdit) {
        node.state = node.state | AccessState::Protected;
    }
}

} // namespace AccessOS
