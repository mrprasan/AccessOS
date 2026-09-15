// AccessOS/src/Core/Speech/SpeechFormatter.cpp

#include "SpeechFormatter.h"

namespace AccessOS {

std::string SpeechFormatter::Format(const AccessNode& node,
                                    const SpeechPolicy& policy)
{
    // Protected fields must never be formatted into speech.
    // This is a hard rule — password field values are never spoken.
    if (node.IsProtected() && !node.value.empty()) {
        // Only speak the name and role, never the value.
    }

    std::string result;

    // ── Name ─────────────────────────────────────────────────────────────────
    if (!node.name.empty()) {
        result += node.name;
    }

    // ── Role ─────────────────────────────────────────────────────────────────
    if (policy.announceRole) {
        const char* roleStr = RoleText(node.role);
        if (roleStr && roleStr[0] != '\0') {
            if (!result.empty()) result += ", ";
            result += roleStr;
        }
    }

    // ── Heading level ────────────────────────────────────────────────────────
    // Heading level is appended to the role: "heading level 2"
    if (node.role == AccessRole::Heading && node.headingLevel > 0) {
        result += " level ";
        result += std::to_string(node.headingLevel);
    }

    // ── State ────────────────────────────────────────────────────────────────
    if (policy.announceState) {
        std::string stateStr = StateText(node, policy);
        if (!stateStr.empty()) {
            if (!result.empty()) result += ", ";
            result += stateStr;
        }
    }

    // ── Value ────────────────────────────────────────────────────────────────
    // Only speak value for non-protected elements and when policy allows.
    if (policy.announceValue && !node.IsProtected() && !node.value.empty()) {
        // Don't repeat value if it was already in the name.
        if (node.value != node.name) {
            if (!result.empty()) result += ", ";
            result += node.value;
        }
    }

    // ── Position ─────────────────────────────────────────────────────────────
    if (policy.announcePosition) {
        std::string pos = PositionText(node);
        if (!pos.empty()) {
            if (!result.empty()) result += ", ";
            result += pos;
        }
    }

    // ── Description ──────────────────────────────────────────────────────────
    if (policy.announceDescription && !node.description.empty()) {
        if (!result.empty()) result += ". ";
        result += node.description;
    }

    return result;
}

const char* SpeechFormatter::RoleText(AccessRole role) noexcept {
    switch (role) {
    case AccessRole::Button:         return "button";
    case AccessRole::SplitButton:    return "split button";
    case AccessRole::ToggleButton:   return "toggle button";
    case AccessRole::CheckBox:       return "checkbox";
    case AccessRole::RadioButton:    return "radio button";
    case AccessRole::ComboBox:       return "combo box";
    case AccessRole::ListBox:        return "list";
    case AccessRole::ListItem:       return "list item";
    case AccessRole::Edit:           return "edit";
    case AccessRole::MultiLineEdit:  return "edit";
    case AccessRole::PasswordEdit:   return "password";
    case AccessRole::StaticText:     return "";          // Text — no role spoken
    case AccessRole::Link:           return "link";
    case AccessRole::Image:          return "image";
    case AccessRole::Heading:        return "heading";   // Level appended separately
    case AccessRole::Table:          return "table";
    case AccessRole::TableRow:       return "row";
    case AccessRole::TableCell:      return "cell";
    case AccessRole::TableColumnHeader: return "column header";
    case AccessRole::TableRowHeader: return "row header";
    case AccessRole::List:           return "list";
    case AccessRole::Menu:           return "menu";
    case AccessRole::MenuBar:        return "menu bar";
    case AccessRole::MenuItem:       return "menu item";
    case AccessRole::TabControl:     return "tab control";
    case AccessRole::Tab:            return "tab";
    case AccessRole::Tree:           return "tree";
    case AccessRole::TreeItem:       return "tree item";
    case AccessRole::Dialog:         return "dialog";
    case AccessRole::Window:         return "window";
    case AccessRole::Pane:           return "pane";
    case AccessRole::Group:          return "group";
    case AccessRole::Document:       return "document";
    case AccessRole::Slider:         return "slider";
    case AccessRole::Spinner:        return "spinner";
    case AccessRole::ProgressBar:    return "progress bar";
    case AccessRole::ScrollBar:      return "scroll bar";
    case AccessRole::ToolBar:        return "tool bar";
    case AccessRole::Tooltip:        return "tooltip";
    case AccessRole::Form:           return "form";
    case AccessRole::Main:           return "main";
    case AccessRole::Navigation:     return "navigation";
    case AccessRole::Banner:         return "banner";
    case AccessRole::Search:         return "search";
    case AccessRole::Alert:          return "alert";
    case AccessRole::AlertDialog:    return "alert dialog";
    case AccessRole::Status:         return "status";
    case AccessRole::Region:         return "region";
    default:                         return "";
    }
}

std::string SpeechFormatter::StateText(const AccessNode& node,
                                       const SpeechPolicy& /*policy*/)
{
    std::string result;

    auto append = [&](const char* text) {
        if (!result.empty()) result += ", ";
        result += text;
    };

    if (HasState(node.state, AccessState::Checked))       append("checked");
    if (HasState(node.state, AccessState::Indeterminate)) append("partially checked");
    if (HasState(node.state, AccessState::Expanded))      append("expanded");
    if (HasState(node.state, AccessState::Collapsed))     append("collapsed");
    if (HasState(node.state, AccessState::Pressed))       append("pressed");
    if (HasState(node.state, AccessState::Selected))      append("selected");
    if (HasState(node.state, AccessState::Disabled))      append("unavailable");
    if (HasState(node.state, AccessState::ReadOnly))      append("read only");
    if (HasState(node.state, AccessState::Required))      append("required");
    if (HasState(node.state, AccessState::MultiLine))     append("multi-line");

    // Protected (password) — state announced as role "password", not as a state string.
    // HasPopup
    if (HasState(node.state, AccessState::HasPopup))      append("has pop-up");

    return result;
}

std::string SpeechFormatter::PositionText(const AccessNode& node) {
    if (!node.positionInSet.has_value() || !node.setSize.has_value()) return {};
    return std::to_string(node.positionInSet.value()) +
           " of " +
           std::to_string(node.setSize.value());
}

} // namespace AccessOS
