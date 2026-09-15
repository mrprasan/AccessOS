// AccessOS/src/Core/Announcement/AnnouncementEngine.cpp

#include "AnnouncementEngine.h"

#include <algorithm>
#include <cctype>

namespace AccessOS {

// ── RoleText ──────────────────────────────────────────────────────────────────

const char* AnnouncementEngine::RoleText(AccessRole role) noexcept {
    switch (role) {
    case AccessRole::Button:            return "button";
    case AccessRole::SplitButton:       return "split button";
    case AccessRole::ToggleButton:      return "toggle button";
    case AccessRole::CheckBox:          return "checkbox";
    case AccessRole::RadioButton:       return "radio button";
    case AccessRole::ComboBox:          return "combo box";
    case AccessRole::ListBox:           return "list";
    case AccessRole::ListItem:          return "list item";
    case AccessRole::Edit:              return "edit";
    case AccessRole::MultiLineEdit:     return "edit";
    case AccessRole::PasswordEdit:      return "password";
    case AccessRole::StaticText:        return "";
    case AccessRole::Link:              return "link";
    case AccessRole::Image:             return "image";
    case AccessRole::Heading:           return "heading";
    case AccessRole::Table:             return "table";
    case AccessRole::TableRow:          return "row";
    case AccessRole::TableCell:         return "cell";
    case AccessRole::TableColumnHeader: return "column header";
    case AccessRole::TableRowHeader:    return "row header";
    case AccessRole::List:              return "list";
    case AccessRole::Menu:              return "menu";
    case AccessRole::MenuBar:           return "menu bar";
    case AccessRole::MenuItem:          return "menu item";
    case AccessRole::TabControl:        return "tab control";
    case AccessRole::Tab:               return "tab";
    case AccessRole::Tree:              return "tree";
    case AccessRole::TreeItem:          return "tree item";
    case AccessRole::Dialog:            return "dialog";
    case AccessRole::AlertDialog:       return "alert dialog";
    case AccessRole::Window:            return "window";
    case AccessRole::Pane:              return "pane";
    case AccessRole::Group:             return "group";
    case AccessRole::Document:          return "document";
    case AccessRole::Slider:            return "slider";
    case AccessRole::Spinner:           return "spinner";
    case AccessRole::ProgressBar:       return "progress bar";
    case AccessRole::ScrollBar:         return "scroll bar";
    case AccessRole::ToolBar:           return "tool bar";
    case AccessRole::Tooltip:           return "tooltip";
    case AccessRole::Form:              return "form";
    case AccessRole::Main:              return "main";
    case AccessRole::Navigation:        return "navigation";
    case AccessRole::Banner:            return "banner";
    case AccessRole::Search:            return "search";
    case AccessRole::Alert:             return "alert";
    case AccessRole::Status:            return "status";
    case AccessRole::Region:            return "region";
    default:                            return "";
    }
}

// ── StateText ─────────────────────────────────────────────────────────────────

std::string AnnouncementEngine::StateText(const AccessNode& node,
                                           const SpeechPolicy& /*policy*/) noexcept
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
    if (HasState(node.state, AccessState::HasPopup))      append("has pop-up");

    return result;
}

// ── ValueText ─────────────────────────────────────────────────────────────────

std::string AnnouncementEngine::ValueText(const AccessNode& node) noexcept {
    // Never speak password values.
    if (node.IsProtected()) return {};
    // Don't repeat value if it equals the name.
    if (!node.value.empty() && node.value != node.name) {
        return node.value;
    }
    return {};
}

// ── PositionText ─────────────────────────────────────────────────────────────

std::string AnnouncementEngine::PositionText(const AccessNode& node) noexcept {
    if (!node.positionInSet.has_value() || !node.setSize.has_value()) return {};
    return std::to_string(node.positionInSet.value()) +
           " of " +
           std::to_string(node.setSize.value());
}

// ── IsRoleSuppressedInContext ─────────────────────────────────────────────────

bool AnnouncementEngine::IsRoleSuppressedInContext(
    AccessRole role, AppContextType ctxType) noexcept
{
    // In Terminal context, container roles add noise — suppress them.
    if (ctxType == AppContextType::Terminal) {
        return role == AccessRole::Window  ||
               role == AccessRole::Pane   ||
               role == AccessRole::Group  ||
               role == AccessRole::Document;
    }
    // In CodeEditor, same container suppression.
    if (ctxType == AppContextType::CodeEditor) {
        return role == AccessRole::Pane  ||
               role == AccessRole::Group ||
               role == AccessRole::Document;
    }
    return false;
}

// ── BuildFocusAnnouncement ───────────────────────────────────────────────────

Announcement AnnouncementEngine::BuildFocusAnnouncement(
    const AccessNode&  node,
    const AppContext&  ctx,
    const SpeechPolicy& policy,
    const std::string& prevTitle) noexcept
{
    Announcement ann;
    ann.cancelPrevious = policy.navigationCancelsNormal;

    // R1: Invalid node → empty.
    if (!node.isValid) return ann;

    // R9: Dialog context — prepend dialog title when we enter a new dialog.
    if (ctx.isModal &&
        !ctx.windowTitle.empty() &&
        ctx.windowTitle != prevTitle)
    {
        ann.Add(AnnouncementPartKind::Context, ctx.windowTitle);
    }

    // R2: Name part — always first (after optional context).
    if (!node.name.empty()) {
        ann.Add(AnnouncementPartKind::Name, node.name);
    }

    // R3: Role part — skip at Minimal verbosity or if suppressed by context.
    if (policy.announceRole &&
        policy.verbosity != VerbosityLevel::Minimal &&
        !IsRoleSuppressedInContext(node.role, ctx.type))
    {
        const char* roleStr = RoleText(node.role);
        if (roleStr && roleStr[0] != '\0') {
            std::string roleText = roleStr;
            // Heading level appended to role.
            if (node.role == AccessRole::Heading && node.headingLevel > 0) {
                roleText += " level ";
                roleText += std::to_string(node.headingLevel);
            }
            ann.Add(AnnouncementPartKind::Role, roleText);
        }
    }

    // R4: State part.
    if (policy.announceState &&
        policy.verbosity != VerbosityLevel::Minimal)
    {
        ann.Add(AnnouncementPartKind::State, StateText(node, policy));
    }

    // R5: Value part.
    if (policy.announceValue) {
        ann.Add(AnnouncementPartKind::Value, ValueText(node));
    }

    // R6: Position part.
    if (policy.announcePosition) {
        ann.Add(AnnouncementPartKind::Position, PositionText(node));
    }

    // R7: Description at Detailed verbosity only.
    if (policy.announceDescription &&
        policy.verbosity == VerbosityLevel::Detailed &&
        !node.description.empty())
    {
        ann.Add(AnnouncementPartKind::Description, node.description);
    }

    return ann;
}

// ── BuildPropertyAnnouncement ────────────────────────────────────────────────

Announcement AnnouncementEngine::BuildPropertyAnnouncement(
    const AccessNode&  node,
    const AppContext&  /*ctx*/,
    const SpeechPolicy& policy) noexcept
{
    Announcement ann;
    ann.priority = SpeechPriority::Normal;
    ann.cancelPrevious = false;

    if (!node.isValid) return ann;

    // For property changes, announce name + changed value/state.
    if (!node.name.empty()) {
        ann.Add(AnnouncementPartKind::Name, node.name);
    }

    if (policy.announceValue) {
        ann.Add(AnnouncementPartKind::Value, ValueText(node));
    }

    if (policy.announceState &&
        policy.verbosity != VerbosityLevel::Minimal)
    {
        ann.Add(AnnouncementPartKind::State, StateText(node, policy));
    }

    return ann;
}

// ── BuildAlertAnnouncement ───────────────────────────────────────────────────

Announcement AnnouncementEngine::BuildAlertAnnouncement(
    const AccessNode&  node,
    const AppContext&  /*ctx*/) noexcept
{
    Announcement ann;
    ann.priority = SpeechPriority::High;
    ann.cancelPrevious = true;

    if (!node.isValid) return ann;

    // Always announce: role prefix ("alert"), then name, then value.
    if (node.role == AccessRole::Alert ||
        node.role == AccessRole::AlertDialog)
    {
        ann.Add(AnnouncementPartKind::Role, RoleText(node.role));
    }

    if (!node.name.empty()) {
        ann.Add(AnnouncementPartKind::Name, node.name);
    }
    if (!node.value.empty() && node.value != node.name) {
        ann.Add(AnnouncementPartKind::Value, node.value);
    }

    return ann;
}

} // namespace AccessOS
