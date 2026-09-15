#pragma once
// EarconId.h — Named audio cue identifiers (ACCESSOS-024)
//
// Each EarconId maps to a distinct short audio cue (earcon).
// The mapping from EarconId → tone parameters lives in EarconManager.

#include <cstdint>
#include <string>

namespace AccessOS {
namespace Audio {

enum class EarconId : uint32_t {
    None          = 0,

    // Focus & navigation
    FocusEnter    = 1,   // element received focus
    FocusLeave    = 2,   // focus left element
    MenuItem      = 3,   // menu item focused
    ListItem      = 4,   // list item focused
    Link          = 5,   // hyperlink focused
    Button        = 6,   // button focused
    Checkbox      = 7,   // checkbox focused
    Combobox      = 8,   // combo box focused
    TextInput     = 9,   // text input focused

    // State changes
    Checked       = 10,  // checkbox/toggle turned on
    Unchecked     = 11,  // checkbox/toggle turned off
    Expanded      = 12,  // tree/accordion expanded
    Collapsed     = 13,  // tree/accordion collapsed
    Selected      = 14,  // item selected in list

    // Alerts & errors
    Alert         = 20,  // alert/notification appeared
    Error         = 21,  // validation error
    Warning       = 22,  // warning
    Success       = 23,  // operation succeeded

    // Reading
    LineStart      = 30, // virtual cursor moved to line start
    LineEnd        = 31, // virtual cursor moved to line end
    DocumentStart  = 32, // virtual cursor at document top
    DocumentEnd    = 33, // virtual cursor at document end

    // Boundary
    Boundary       = 40, // hit a navigation boundary (cannot go further)
};

// Human-readable name for diagnostics / logging
inline std::string EarconName(EarconId id) {
    switch (id) {
    case EarconId::None:          return "None";
    case EarconId::FocusEnter:    return "FocusEnter";
    case EarconId::FocusLeave:    return "FocusLeave";
    case EarconId::MenuItem:      return "MenuItem";
    case EarconId::ListItem:      return "ListItem";
    case EarconId::Link:          return "Link";
    case EarconId::Button:        return "Button";
    case EarconId::Checkbox:      return "Checkbox";
    case EarconId::Combobox:      return "Combobox";
    case EarconId::TextInput:     return "TextInput";
    case EarconId::Checked:       return "Checked";
    case EarconId::Unchecked:     return "Unchecked";
    case EarconId::Expanded:      return "Expanded";
    case EarconId::Collapsed:     return "Collapsed";
    case EarconId::Selected:      return "Selected";
    case EarconId::Alert:         return "Alert";
    case EarconId::Error:         return "Error";
    case EarconId::Warning:       return "Warning";
    case EarconId::Success:       return "Success";
    case EarconId::LineStart:     return "LineStart";
    case EarconId::LineEnd:       return "LineEnd";
    case EarconId::DocumentStart: return "DocumentStart";
    case EarconId::DocumentEnd:   return "DocumentEnd";
    case EarconId::Boundary:      return "Boundary";
    default:                      return "Unknown";
    }
}

} // namespace Audio
} // namespace AccessOS
