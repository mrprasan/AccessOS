#pragma once
// VirtualNode.h — Virtual cursor document node (ACCESSOS-026)
//
// A VirtualNode represents one piece of linearised accessible content.
// The VirtualDocument is a flat, ordered sequence of VirtualNodes.

#include <string>
#include <cstdint>

namespace AccessOS {
namespace Browse {

// Semantic role of a virtual node (simplified, browse-mode focused)
enum class VirtualRole : uint32_t {
    Text        = 0,
    Heading1    = 1,
    Heading2    = 2,
    Heading3    = 3,
    Heading4    = 4,
    Heading5    = 5,
    Heading6    = 6,
    Link        = 10,
    Button      = 11,
    Checkbox    = 12,
    Combobox    = 13,
    TextInput   = 14,
    ListItem    = 20,
    List        = 21,
    Table       = 22,
    Image       = 30,
    Landmark    = 40, // nav, main, banner, contentinfo, etc.
    Separator   = 50,
    Unknown     = 99,
};

// Is the role a heading?
inline bool IsHeading(VirtualRole r) noexcept {
    auto v = static_cast<uint32_t>(r);
    return v >= 1 && v <= 6;
}

// Is the role a form control?
inline bool IsFormControl(VirtualRole r) noexcept {
    return r == VirtualRole::Button   ||
           r == VirtualRole::Checkbox ||
           r == VirtualRole::Combobox ||
           r == VirtualRole::TextInput;
}

// Is the role a landmark?
inline bool IsLandmark(VirtualRole r) noexcept {
    return r == VirtualRole::Landmark;
}

// One node in the virtual document
struct VirtualNode {
    std::string text;          // readable text (accessible name or text content)
    VirtualRole role           = VirtualRole::Text;
    uint32_t    headingLevel   = 0; // 1–6 for headings, 0 otherwise
    std::string landmarkLabel; // e.g. "navigation", "main" for landmarks
    bool        isEditable     = false;
    bool        isSelected     = false;
    uint32_t    nodeIndex      = 0; // position in flat document (0-based)
};

} // namespace Browse
} // namespace AccessOS
