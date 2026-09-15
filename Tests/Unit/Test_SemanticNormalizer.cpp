// Tests/Unit/Test_SemanticNormalizer.cpp
// Unit tests for SemanticNormalizer and SemanticModel.

#include <gtest/gtest.h>
#include "Semantic/SemanticNormalizer.h"
#include "Semantic/SemanticModel.h"
#include "Events/IEventListener.h"

using namespace AccessOS;

static AccessNode MakeNode(AccessRole role, const std::string& name,
                           const std::string& value = {}) {
    AccessNode n;
    n.role  = role;
    n.name  = name;
    n.value = value;
    n.id    = 1;
    return n;
}

// ─── Name normalization ───────────────────────────────────────────────────────

TEST(SemanticNormalizerTest, TrimsLeadingAndTrailingWhitespace) {
    auto n = SemanticNormalizer::Normalize(MakeNode(AccessRole::Button, "  Submit  "));
    EXPECT_EQ(n.name, "Submit");
}

TEST(SemanticNormalizerTest, TrimsDescription) {
    AccessNode node = MakeNode(AccessRole::Button, "OK");
    node.description = "  Confirm action  ";
    auto n = SemanticNormalizer::Normalize(node);
    EXPECT_EQ(n.description, "Confirm action");
}

TEST(SemanticNormalizerTest, TrimsHelpText) {
    AccessNode node = MakeNode(AccessRole::Button, "OK");
    node.helpText = "\tPress to confirm\t";
    auto n = SemanticNormalizer::Normalize(node);
    EXPECT_EQ(n.helpText, "Press to confirm");
}

TEST(SemanticNormalizerTest, EmptyNameRemainsEmptyForNonStaticText) {
    auto n = SemanticNormalizer::Normalize(MakeNode(AccessRole::Button, ""));
    EXPECT_TRUE(n.name.empty());
}

TEST(SemanticNormalizerTest, StaticTextFallsBackToValueWhenNameEmpty) {
    auto n = SemanticNormalizer::Normalize(
        MakeNode(AccessRole::StaticText, "", "Hello World"));
    EXPECT_EQ(n.name, "Hello World");
}

TEST(SemanticNormalizerTest, StaticTextKeepsNameWhenPresent) {
    auto n = SemanticNormalizer::Normalize(
        MakeNode(AccessRole::StaticText, "Label", "Fallback"));
    EXPECT_EQ(n.name, "Label");
}

// ─── State normalization ──────────────────────────────────────────────────────

TEST(SemanticNormalizerTest, EditRoleGetsMultiLineState) {
    auto n = SemanticNormalizer::Normalize(MakeNode(AccessRole::Edit, "Notes"));
    EXPECT_TRUE(HasState(n.state, AccessState::MultiLine));
}

TEST(SemanticNormalizerTest, MultiLineEditRoleGetsMultiLineState) {
    auto n = SemanticNormalizer::Normalize(MakeNode(AccessRole::MultiLineEdit, "Body"));
    EXPECT_TRUE(HasState(n.state, AccessState::MultiLine));
}

TEST(SemanticNormalizerTest, ButtonDoesNotGetMultiLineState) {
    auto n = SemanticNormalizer::Normalize(MakeNode(AccessRole::Button, "Submit"));
    EXPECT_FALSE(HasState(n.state, AccessState::MultiLine));
}

TEST(SemanticNormalizerTest, PasswordEditGetsProtectedState) {
    auto n = SemanticNormalizer::Normalize(MakeNode(AccessRole::PasswordEdit, "Password"));
    EXPECT_TRUE(HasState(n.state, AccessState::Protected));
}

TEST(SemanticNormalizerTest, NormalizationPreservesExistingState) {
    AccessNode node = MakeNode(AccessRole::Button, "Save");
    node.state = AccessState::Focused | AccessState::Focusable;
    auto n = SemanticNormalizer::Normalize(node);
    EXPECT_TRUE(HasState(n.state, AccessState::Focused));
    EXPECT_TRUE(HasState(n.state, AccessState::Focusable));
}

// ─── SemanticModel integration ────────────────────────────────────────────────

TEST(SemanticModelTest, FocusChangedEventUpdatesCache) {
    SemanticModel model;

    AccessEvent evt;
    evt.type            = AccessEventType::FocusChanged;
    evt.element.id      = 100;
    evt.element.role    = AccessRole::Edit;
    evt.element.name    = "  Username  ";
    evt.element.isValid = true;

    model.OnEvent(evt);

    auto focused = model.GetFocused();
    ASSERT_TRUE(focused.has_value());
    EXPECT_EQ(focused->id,   100u);
    EXPECT_EQ(focused->name, "Username");   // trimmed by normalizer
    EXPECT_TRUE(HasState(focused->state, AccessState::Focused));
    EXPECT_TRUE(HasState(focused->state, AccessState::MultiLine));  // Edit → MultiLine
}

TEST(SemanticModelTest, FocusObserverIsCalledOnFocusChange) {
    SemanticModel model;
    bool called = false;
    std::string observedName;

    model.AddFocusObserver([&](const AccessNode& node) {
        called       = true;
        observedName = node.name;
    });

    AccessEvent evt;
    evt.type            = AccessEventType::FocusChanged;
    evt.element.id      = 200;
    evt.element.role    = AccessRole::Button;
    evt.element.name    = "Submit";
    evt.element.isValid = true;

    model.OnEvent(evt);

    EXPECT_TRUE(called);
    EXPECT_EQ(observedName, "Submit");
}

TEST(SemanticModelTest, ZeroIdFocusEventIsIgnored) {
    SemanticModel model;

    AccessEvent evt;
    evt.type         = AccessEventType::FocusChanged;
    evt.element.id   = 0;   // Invalid ID
    evt.element.name = "Ghost";

    model.OnEvent(evt);

    EXPECT_FALSE(model.GetFocused().has_value());
    EXPECT_EQ(model.CacheSize(), 0u);
}

TEST(SemanticModelTest, PropertyChangedUpdatesExistingNode) {
    SemanticModel model;

    // First establish focus
    AccessEvent focus;
    focus.type            = AccessEventType::FocusChanged;
    focus.element.id      = 10;
    focus.element.role    = AccessRole::Edit;
    focus.element.name    = "Search";
    focus.element.isValid = true;
    model.OnEvent(focus);

    // Then update the name via property change
    AccessEvent prop;
    prop.type            = AccessEventType::NameChanged;
    prop.element.id      = 10;
    prop.element.role    = AccessRole::Edit;
    prop.element.name    = "Search Box";
    prop.element.isValid = true;
    model.OnEvent(prop);

    auto node = model.GetNode(10);
    ASSERT_TRUE(node.has_value());
    EXPECT_EQ(node->name, "Search Box");
}

TEST(SemanticModelTest, ClearResetsCache) {
    SemanticModel model;

    AccessEvent evt;
    evt.type            = AccessEventType::FocusChanged;
    evt.element.id      = 55;
    evt.element.role    = AccessRole::Button;
    evt.element.name    = "Go";
    evt.element.isValid = true;
    model.OnEvent(evt);

    EXPECT_GT(model.CacheSize(), 0u);
    model.Clear();
    EXPECT_EQ(model.CacheSize(), 0u);
    EXPECT_FALSE(model.GetFocused().has_value());
}
