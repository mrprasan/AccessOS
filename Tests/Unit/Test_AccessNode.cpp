// Tests/Unit/Test_AccessNode.cpp
// Unit tests for AccessOS::AccessNode semantic model.

#include <gtest/gtest.h>
#include "Semantic/AccessNode.h"

using namespace AccessOS;

// ─── AccessState flag operations ─────────────────────────────────────────────

TEST(AccessStateTest, HasStateTrueWhenFlagSet) {
    AccessState state = AccessState::Focused | AccessState::Focusable;
    EXPECT_TRUE(HasState(state, AccessState::Focused));
    EXPECT_TRUE(HasState(state, AccessState::Focusable));
}

TEST(AccessStateTest, HasStateFalseWhenFlagNotSet) {
    AccessState state = AccessState::Focused;
    EXPECT_FALSE(HasState(state, AccessState::Disabled));
    EXPECT_FALSE(HasState(state, AccessState::Hidden));
}

TEST(AccessStateTest, NoneStateHasNoFlags) {
    AccessState state = AccessState::None;
    EXPECT_FALSE(HasState(state, AccessState::Focused));
    EXPECT_FALSE(HasState(state, AccessState::Disabled));
    EXPECT_FALSE(HasState(state, AccessState::Protected));
}

TEST(AccessStateTest, MultipleStateFlagsCanBeSet) {
    AccessState state =
        AccessState::Focused |
        AccessState::Required |
        AccessState::MultiLine;

    EXPECT_TRUE(HasState(state, AccessState::Focused));
    EXPECT_TRUE(HasState(state, AccessState::Required));
    EXPECT_TRUE(HasState(state, AccessState::MultiLine));
    EXPECT_FALSE(HasState(state, AccessState::Disabled));
}

// ─── AccessNode default state ────────────────────────────────────────────────

TEST(AccessNodeTest, DefaultConstructedNodeHasExpectedDefaults) {
    AccessNode node;
    EXPECT_EQ(node.id,           0u);
    EXPECT_EQ(node.role,         AccessRole::Unknown);
    EXPECT_TRUE(node.name.empty());
    EXPECT_TRUE(node.description.empty());
    EXPECT_TRUE(node.value.empty());
    EXPECT_EQ(node.state,        AccessState::None);
    EXPECT_EQ(node.headingLevel, 0);
    EXPECT_EQ(node.parentId,     0u);
    EXPECT_TRUE(node.childIds.empty());
    EXPECT_EQ(node.provider,     AccessProvider::Unknown);
    EXPECT_TRUE(node.isValid);
    EXPECT_FALSE(node.positionInSet.has_value());
    EXPECT_FALSE(node.setSize.has_value());
    EXPECT_FALSE(node.textContent.has_value());
}

TEST(AccessNodeTest, IsFocusedReflectsState) {
    AccessNode node;
    EXPECT_FALSE(node.IsFocused());

    node.state = AccessState::Focused;
    EXPECT_TRUE(node.IsFocused());
}

TEST(AccessNodeTest, IsDisabledReflectsState) {
    AccessNode node;
    EXPECT_FALSE(node.IsDisabled());

    node.state = AccessState::Disabled;
    EXPECT_TRUE(node.IsDisabled());
}

TEST(AccessNodeTest, IsHiddenReflectsState) {
    AccessNode node;
    EXPECT_FALSE(node.IsHidden());

    node.state = AccessState::Hidden;
    EXPECT_TRUE(node.IsHidden());
}

TEST(AccessNodeTest, IsProtectedReflectsState) {
    AccessNode node;
    EXPECT_FALSE(node.IsProtected());

    node.state = AccessState::Protected;
    EXPECT_TRUE(node.IsProtected());
}

// ─── AccessBounds ────────────────────────────────────────────────────────────

TEST(AccessBoundsTest, DefaultBoundsIsEmpty) {
    AccessBounds bounds;
    EXPECT_TRUE(bounds.IsEmpty());
}

TEST(AccessBoundsTest, NonZeroDimensionsNotEmpty) {
    AccessBounds bounds{ 10, 20, 100, 50 };
    EXPECT_FALSE(bounds.IsEmpty());
}

TEST(AccessBoundsTest, ZeroWidthIsEmpty) {
    AccessBounds bounds{ 10, 20, 0, 50 };
    EXPECT_TRUE(bounds.IsEmpty());
}

// ─── AccessNode population ───────────────────────────────────────────────────

TEST(AccessNodeTest, PopulatedNodeRetainsAllFields) {
    AccessNode node;
    node.id              = 12345;
    node.role            = AccessRole::Button;
    node.name            = "Submit";
    node.description     = "Submits the form";
    node.value           = "";
    node.state           = AccessState::Focused | AccessState::Focusable;
    node.bounds          = { 100, 200, 80, 30 };
    node.provider        = AccessProvider::UIAutomation;
    node.applicationName = "TestApp";
    node.windowTitle     = "Test Window";
    node.processId       = 1234;

    EXPECT_EQ(node.id,              12345u);
    EXPECT_EQ(node.role,            AccessRole::Button);
    EXPECT_EQ(node.name,            "Submit");
    EXPECT_EQ(node.description,     "Submits the form");
    EXPECT_TRUE(node.IsFocused());
    EXPECT_EQ(node.provider,        AccessProvider::UIAutomation);
    EXPECT_EQ(node.applicationName, "TestApp");
    EXPECT_EQ(node.processId,       1234u);
    EXPECT_FALSE(node.bounds.IsEmpty());
}
