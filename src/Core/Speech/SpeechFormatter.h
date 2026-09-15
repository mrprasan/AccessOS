// AccessOS/src/Core/Speech/SpeechFormatter.h
//
// Converts an AccessNode into a speakable string according to SpeechPolicy.
//
// Why: All text-to-speech formatting must go through one point.
//      No navigation, event, or UI code should construct speech strings.
//      Rules like "Button, Submit" or "Edit, multi-line" live here.
//
// Threading: Stateless — safe to call from any thread.

#pragma once

#include "../Semantic/AccessNode.h"
#include "SpeechPolicy.h"
#include <string>

namespace AccessOS {

class SpeechFormatter {
public:
    // Format an AccessNode into a speakable announcement string.
    // Follows the examples from Section 13 of the master instruction:
    //   Button:   "Submit, button"
    //   Checkbox: "Remember me, checkbox, checked"
    //   Heading:  "Accessibility Testing, heading level 2"
    //   Edit:     "Username, edit"
    static std::string Format(const AccessNode& node, const SpeechPolicy& policy);

private:
    // Returns the spoken role string for a given AccessRole.
    static const char* RoleText(AccessRole role) noexcept;

    // Returns spoken state text for the given node (checked, expanded, etc.)
    static std::string StateText(const AccessNode& node, const SpeechPolicy& policy);

    // Returns position text if positionInSet and setSize are set.
    static std::string PositionText(const AccessNode& node);
};

} // namespace AccessOS
