// Tests/Unit/Test_UIARoleMap.cpp
// Unit tests for UIARoleMap — verifies the UIA ControlType to AccessRole mapping.
// These tests do not require a live UIA session.

#include <gtest/gtest.h>
#include "Accessibility/UIAutomation/UIARoleMap.h"
#include <uiautomation.h>

using namespace AccessOS;
using namespace AccessOS::UIARoleMap;

TEST(UIARoleMapTest, ButtonMapsToButton) {
    EXPECT_EQ(FromControlType(UIA_ButtonControlTypeId), AccessRole::Button);
}

TEST(UIARoleMapTest, CheckBoxMapsToCheckBox) {
    EXPECT_EQ(FromControlType(UIA_CheckBoxControlTypeId), AccessRole::CheckBox);
}

TEST(UIARoleMapTest, EditMapsToEdit) {
    EXPECT_EQ(FromControlType(UIA_EditControlTypeId), AccessRole::Edit);
}

TEST(UIARoleMapTest, HyperlinkMapsToLink) {
    EXPECT_EQ(FromControlType(UIA_HyperlinkControlTypeId), AccessRole::Link);
}

TEST(UIARoleMapTest, ImageMapsToImage) {
    EXPECT_EQ(FromControlType(UIA_ImageControlTypeId), AccessRole::Image);
}

TEST(UIARoleMapTest, ListMapsToListBox) {
    EXPECT_EQ(FromControlType(UIA_ListControlTypeId), AccessRole::ListBox);
}

TEST(UIARoleMapTest, ListItemMapsToListItem) {
    EXPECT_EQ(FromControlType(UIA_ListItemControlTypeId), AccessRole::ListItem);
}

TEST(UIARoleMapTest, MenuMapsToMenu) {
    EXPECT_EQ(FromControlType(UIA_MenuControlTypeId), AccessRole::Menu);
}

TEST(UIARoleMapTest, MenuBarMapsToMenuBar) {
    EXPECT_EQ(FromControlType(UIA_MenuBarControlTypeId), AccessRole::MenuBar);
}

TEST(UIARoleMapTest, MenuItemMapsToMenuItem) {
    EXPECT_EQ(FromControlType(UIA_MenuItemControlTypeId), AccessRole::MenuItem);
}

TEST(UIARoleMapTest, RadioButtonMapsToRadioButton) {
    EXPECT_EQ(FromControlType(UIA_RadioButtonControlTypeId), AccessRole::RadioButton);
}

TEST(UIARoleMapTest, TabControlMapsToTabControl) {
    EXPECT_EQ(FromControlType(UIA_TabControlTypeId), AccessRole::TabControl);
}

TEST(UIARoleMapTest, TabItemMapsToTab) {
    EXPECT_EQ(FromControlType(UIA_TabItemControlTypeId), AccessRole::Tab);
}

TEST(UIARoleMapTest, TextMapsToStaticText) {
    EXPECT_EQ(FromControlType(UIA_TextControlTypeId), AccessRole::StaticText);
}

TEST(UIARoleMapTest, TreeMapsToTree) {
    EXPECT_EQ(FromControlType(UIA_TreeControlTypeId), AccessRole::Tree);
}

TEST(UIARoleMapTest, TreeItemMapsToTreeItem) {
    EXPECT_EQ(FromControlType(UIA_TreeItemControlTypeId), AccessRole::TreeItem);
}

TEST(UIARoleMapTest, WindowMapsToWindow) {
    EXPECT_EQ(FromControlType(UIA_WindowControlTypeId), AccessRole::Window);
}

TEST(UIARoleMapTest, DocumentMapsToDocument) {
    EXPECT_EQ(FromControlType(UIA_DocumentControlTypeId), AccessRole::Document);
}

TEST(UIARoleMapTest, TableMapsToTable) {
    EXPECT_EQ(FromControlType(UIA_TableControlTypeId), AccessRole::Table);
}

TEST(UIARoleMapTest, DataGridMapsToTable) {
    EXPECT_EQ(FromControlType(UIA_DataGridControlTypeId), AccessRole::Table);
}

TEST(UIARoleMapTest, PaneMapsToPane) {
    EXPECT_EQ(FromControlType(UIA_PaneControlTypeId), AccessRole::Pane);
}

TEST(UIARoleMapTest, GroupMapsToGroup) {
    EXPECT_EQ(FromControlType(UIA_GroupControlTypeId), AccessRole::Group);
}

TEST(UIARoleMapTest, UnknownControlTypeReturnsUnknown) {
    // 0 is not a valid UIA ControlType — must return Unknown, not crash.
    EXPECT_EQ(FromControlType(0), AccessRole::Unknown);
}

TEST(UIARoleMapTest, CustomControlTypeReturnsUnknown) {
    EXPECT_EQ(FromControlType(UIA_CustomControlTypeId), AccessRole::Unknown);
}
