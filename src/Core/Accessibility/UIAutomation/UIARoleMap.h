// AccessOS/src/Core/Accessibility/UIAutomation/UIARoleMap.h
//
// Maps UIA ControlType identifiers to AccessOS AccessRole values.
//
// Why: UIA ControlType IDs are Windows-specific integer constants.
//      The semantic engine must never depend on these constants directly.
//      This mapping table is the single point of conversion.
//
// Reference: https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-controltype-ids

#pragma once

#include "../../Semantic/AccessNode.h"
#include "UIAIncludes.h"

namespace AccessOS {
namespace UIARoleMap {

// Returns the AccessRole for a given UIA ControlType ID.
// Returns AccessRole::Unknown for unmapped types — callers must handle this.
inline AccessRole FromControlType(CONTROLTYPEID ct) noexcept {
    switch (ct) {
    case UIA_ButtonControlTypeId:           return AccessRole::Button;
    case UIA_CalendarControlTypeId:         return AccessRole::Unknown;      // No direct mapping yet
    case UIA_CheckBoxControlTypeId:         return AccessRole::CheckBox;
    case UIA_ComboBoxControlTypeId:         return AccessRole::ComboBox;
    case UIA_EditControlTypeId:             return AccessRole::Edit;
    case UIA_HyperlinkControlTypeId:        return AccessRole::Link;
    case UIA_ImageControlTypeId:            return AccessRole::Image;
    case UIA_ListItemControlTypeId:         return AccessRole::ListItem;
    case UIA_ListControlTypeId:             return AccessRole::ListBox;
    case UIA_MenuControlTypeId:             return AccessRole::Menu;
    case UIA_MenuBarControlTypeId:          return AccessRole::MenuBar;
    case UIA_MenuItemControlTypeId:         return AccessRole::MenuItem;
    case UIA_ProgressBarControlTypeId:      return AccessRole::ProgressBar;
    case UIA_RadioButtonControlTypeId:      return AccessRole::RadioButton;
    case UIA_ScrollBarControlTypeId:        return AccessRole::ScrollBar;
    case UIA_SliderControlTypeId:           return AccessRole::Slider;
    case UIA_SpinnerControlTypeId:          return AccessRole::Spinner;
    case UIA_StatusBarControlTypeId:        return AccessRole::Status;
    case UIA_TabControlTypeId:              return AccessRole::TabControl;
    case UIA_TabItemControlTypeId:          return AccessRole::Tab;
    case UIA_TextControlTypeId:             return AccessRole::StaticText;
    case UIA_ToolBarControlTypeId:          return AccessRole::ToolBar;
    case UIA_ToolTipControlTypeId:          return AccessRole::Tooltip;
    case UIA_TreeControlTypeId:             return AccessRole::Tree;
    case UIA_TreeItemControlTypeId:         return AccessRole::TreeItem;
    case UIA_CustomControlTypeId:           return AccessRole::Unknown;
    case UIA_GroupControlTypeId:            return AccessRole::Group;
    case UIA_ThumbControlTypeId:            return AccessRole::Unknown;
    case UIA_DataGridControlTypeId:         return AccessRole::Table;
    case UIA_DataItemControlTypeId:         return AccessRole::TableCell;
    case UIA_DocumentControlTypeId:         return AccessRole::Document;
    case UIA_SplitButtonControlTypeId:      return AccessRole::SplitButton;
    case UIA_WindowControlTypeId:           return AccessRole::Window;
    case UIA_PaneControlTypeId:             return AccessRole::Pane;
    case UIA_HeaderControlTypeId:           return AccessRole::TableColumnHeader;
    case UIA_HeaderItemControlTypeId:       return AccessRole::TableColumnHeader;
    case UIA_TableControlTypeId:            return AccessRole::Table;
    case UIA_TitleBarControlTypeId:         return AccessRole::Pane;
    case UIA_SeparatorControlTypeId:        return AccessRole::Separator;
    case UIA_SemanticZoomControlTypeId:     return AccessRole::Unknown;
    case UIA_AppBarControlTypeId:           return AccessRole::ToolBar;
    default:                                return AccessRole::Unknown;
    }
}

} // namespace UIARoleMap
} // namespace AccessOS
