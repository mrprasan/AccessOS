// Tests/Unit/Test_NavigationEngine.cpp
// Unit tests for NavigationEngine — basic and semantic navigation.
// No live UIA, no keyboard, no speech required.
// Drives SemanticModel via OnEvent so the cache is populated
// the same way the real system would populate it.

#include <gtest/gtest.h>
#include "Navigation/NavigationEngine.h"
#include "Semantic/SemanticModel.h"
#include "Events/IEventListener.h"

#include <vector>
#include <string>

using namespace AccessOS;

// ─── Helpers ─────────────────────────────────────────────────────────────────

static AccessEvent MakeFocusEvent(uint64_t id, AccessRole role,
                                   const std::string& name,
                                   uint64_t parentId = 0) {
    AccessEvent evt;
    evt.type              = AccessEventType::FocusChanged;
    evt.element.id        = id;
    evt.element.role      = role;
    evt.element.name      = name;
    evt.element.parentId  = parentId;
    evt.element.processId = 1;
    evt.element.isValid   = true;
    evt.timestampMs       = 0;
    return evt;
}

static AccessEvent MakeStructureEvent(uint64_t id, AccessRole role,
                                       const std::string& name,
                                       uint64_t parentId = 0) {
    AccessEvent evt;
    evt.type              = AccessEventType::StructureChanged;
    evt.element.id        = id;
    evt.element.role      = role;
    evt.element.name      = name;
    evt.element.parentId  = parentId;
    evt.element.processId = 1;
    evt.element.isValid   = true;
    evt.timestampMs       = 0;
    return evt;
}

// Populate a model with N nodes via StructureChanged then focus one.
// IDs are 1..N; the focused node is set last via FocusChanged.
struct NavFixture {
    SemanticModel    model;
    NavigationEngine engine{ &model };

    // Add nodes 1..count with given role, then focus the given ID.
    void Populate(int count, AccessRole role, uint64_t focusId) {
        for (int i = 1; i <= count; ++i) {
            model.OnEvent(MakeStructureEvent(
                static_cast<uint64_t>(i), role, "Node" + std::to_string(i)));
        }
        model.OnEvent(MakeFocusEvent(focusId, role, "Node" + std::to_string(focusId)));
    }
};

// ─── GetFocused ───────────────────────────────────────────────────────────────

TEST(NavigationEngineTest, GetFocusedReturnsCurrentFocus) {
    NavFixture f;
    f.Populate(3, AccessRole::Button, 2);
    auto focused = f.engine.GetFocused();
    ASSERT_TRUE(focused.has_value());
    EXPECT_EQ(focused->id, 2u);
}

TEST(NavigationEngineTest, GetFocusedNullModelReturnsNullopt) {
    NavigationEngine engine(nullptr);
    EXPECT_FALSE(engine.GetFocused().has_value());
}

// ─── Navigate — Next / Previous ──────────────────────────────────────────────

TEST(NavigationEngineTest, NavigateNextReturnsNextSibling) {
    NavFixture f;
    f.Populate(3, AccessRole::Button, 1);  // Focus = node 1
    auto next = f.engine.Navigate(NavigationDirection::Next);
    ASSERT_TRUE(next.has_value());
    EXPECT_EQ(next->id, 2u);
}

TEST(NavigationEngineTest, NavigatePreviousReturnsPrevSibling) {
    NavFixture f;
    f.Populate(3, AccessRole::Button, 3);  // Focus = node 3
    auto prev = f.engine.Navigate(NavigationDirection::Previous);
    ASSERT_TRUE(prev.has_value());
    EXPECT_EQ(prev->id, 2u);
}

TEST(NavigationEngineTest, NavigateNextAtEndReturnsNullopt) {
    NavFixture f;
    f.Populate(3, AccessRole::Button, 3);  // Focus = last node
    EXPECT_FALSE(f.engine.Navigate(NavigationDirection::Next).has_value());
}

TEST(NavigationEngineTest, NavigatePrevAtBeginReturnsNullopt) {
    NavFixture f;
    f.Populate(3, AccessRole::Button, 1);  // Focus = first node
    EXPECT_FALSE(f.engine.Navigate(NavigationDirection::Previous).has_value());
}

TEST(NavigationEngineTest, NavigateEmptyCacheReturnsNullopt) {
    SemanticModel    model;
    NavigationEngine engine(&model);
    EXPECT_FALSE(engine.Navigate(NavigationDirection::Next).has_value());
}

// ─── GetParent ────────────────────────────────────────────────────────────────

TEST(NavigationEngineTest, GetParentReturnsParentNode) {
    SemanticModel model;
    NavigationEngine engine(&model);

    // Parent node.
    model.OnEvent(MakeStructureEvent(10, AccessRole::Group, "Parent"));
    // Child node with parentId = 10.
    model.OnEvent(MakeFocusEvent(20, AccessRole::Button, "Child", /*parentId=*/10));

    auto parent = engine.GetParent();
    ASSERT_TRUE(parent.has_value());
    EXPECT_EQ(parent->id, 10u);
}

TEST(NavigationEngineTest, GetParentNoParentIdReturnsNullopt) {
    NavFixture f;
    f.Populate(1, AccessRole::Button, 1);  // parentId = 0
    EXPECT_FALSE(f.engine.GetParent().has_value());
}

// ─── GetFirstChild / GetLastChild ─────────────────────────────────────────────

TEST(NavigationEngineTest, GetFirstChildNoChildrenReturnsNullopt) {
    NavFixture f;
    f.Populate(1, AccessRole::Button, 1);
    EXPECT_FALSE(f.engine.GetFirstChild().has_value());
}

TEST(NavigationEngineTest, GetLastChildNoChildrenReturnsNullopt) {
    NavFixture f;
    f.Populate(1, AccessRole::Button, 1);
    EXPECT_FALSE(f.engine.GetLastChild().has_value());
}

// ─── AllOfRole ────────────────────────────────────────────────────────────────

TEST(NavigationEngineTest, AllOfRoleReturnsMatchingNodes) {
    SemanticModel    model;
    NavigationEngine engine(&model);

    model.OnEvent(MakeStructureEvent(1, AccessRole::Button,  "B1"));
    model.OnEvent(MakeStructureEvent(2, AccessRole::Heading, "H1"));
    model.OnEvent(MakeStructureEvent(3, AccessRole::Button,  "B2"));
    model.OnEvent(MakeFocusEvent    (4, AccessRole::Link,    "L1"));

    const auto buttons = engine.AllOfRole(AccessRole::Button);
    EXPECT_EQ(buttons.size(), 2u);

    const auto headings = engine.AllOfRole(AccessRole::Heading);
    EXPECT_EQ(headings.size(), 1u);
}

TEST(NavigationEngineTest, AllOfRoleEmptyWhenNoMatch) {
    NavFixture f;
    f.Populate(3, AccessRole::Button, 1);
    EXPECT_TRUE(f.engine.AllOfRole(AccessRole::Heading).empty());
}

// ─── Semantic navigation: FindNext ───────────────────────────────────────────

TEST(NavigationEngineTest, FindNextHeadingSkipsOtherRoles) {
    SemanticModel    model;
    NavigationEngine engine(&model);

    model.OnEvent(MakeStructureEvent(1, AccessRole::Button,  "B1"));
    model.OnEvent(MakeStructureEvent(2, AccessRole::Heading, "H1"));
    model.OnEvent(MakeFocusEvent    (1, AccessRole::Button,  "B1"));  // Focus = 1

    auto heading = engine.NextHeading();
    ASSERT_TRUE(heading.has_value());
    EXPECT_EQ(heading->id, 2u);
}

TEST(NavigationEngineTest, FindNextWrapsAround) {
    SemanticModel    model;
    NavigationEngine engine(&model);

    model.OnEvent(MakeStructureEvent(1, AccessRole::Heading, "H1"));
    model.OnEvent(MakeStructureEvent(2, AccessRole::Button,  "B1"));
    model.OnEvent(MakeFocusEvent    (2, AccessRole::Button,  "B1"));  // Focus = last

    // Heading is before the focused element — wrap-around must find it.
    auto heading = engine.NextHeading();
    ASSERT_TRUE(heading.has_value());
    EXPECT_EQ(heading->id, 1u);
}

TEST(NavigationEngineTest, PrevHeadingSearchesBackward) {
    SemanticModel    model;
    NavigationEngine engine(&model);

    model.OnEvent(MakeStructureEvent(1, AccessRole::Heading, "H1"));
    model.OnEvent(MakeStructureEvent(2, AccessRole::Button,  "B1"));
    model.OnEvent(MakeFocusEvent    (2, AccessRole::Button,  "B1"));  // Focus = 2

    auto heading = engine.PrevHeading();
    ASSERT_TRUE(heading.has_value());
    EXPECT_EQ(heading->id, 1u);
}

TEST(NavigationEngineTest, FindNextReturnsNulloptWhenRoleAbsent) {
    NavFixture f;
    f.Populate(3, AccessRole::Button, 2);
    EXPECT_FALSE(f.engine.NextHeading().has_value());
    EXPECT_FALSE(f.engine.NextLink().has_value());
}

// ─── Convenience wrappers ─────────────────────────────────────────────────────

TEST(NavigationEngineTest, NextLinkAndPrevLink) {
    SemanticModel    model;
    NavigationEngine engine(&model);

    model.OnEvent(MakeStructureEvent(1, AccessRole::Link, "L1"));
    model.OnEvent(MakeStructureEvent(3, AccessRole::Link, "L2"));
    model.OnEvent(MakeFocusEvent    (2, AccessRole::Button, "B1"));

    auto nextLink = engine.NextLink();
    ASSERT_TRUE(nextLink.has_value());
    EXPECT_EQ(nextLink->id, 3u);

    auto prevLink = engine.PrevLink();
    ASSERT_TRUE(prevLink.has_value());
    EXPECT_EQ(prevLink->id, 1u);
}

TEST(NavigationEngineTest, NextButtonAndPrevButton) {
    SemanticModel    model;
    NavigationEngine engine(&model);

    model.OnEvent(MakeStructureEvent(1, AccessRole::Button, "Btn1"));
    model.OnEvent(MakeStructureEvent(3, AccessRole::Button, "Btn2"));
    model.OnEvent(MakeFocusEvent    (2, AccessRole::Heading, "H1"));

    ASSERT_TRUE(engine.NextButton().has_value());
    EXPECT_EQ(engine.NextButton()->id, 3u);

    ASSERT_TRUE(engine.PrevButton().has_value());
    EXPECT_EQ(engine.PrevButton()->id, 1u);
}

TEST(NavigationEngineTest, NextFormFieldAndPrevFormField) {
    SemanticModel    model;
    NavigationEngine engine(&model);

    model.OnEvent(MakeStructureEvent(1, AccessRole::Edit, "Field1"));
    model.OnEvent(MakeStructureEvent(3, AccessRole::Edit, "Field2"));
    model.OnEvent(MakeFocusEvent    (2, AccessRole::Button, "Btn"));

    ASSERT_TRUE(engine.NextFormField().has_value());
    EXPECT_EQ(engine.NextFormField()->id, 3u);

    ASSERT_TRUE(engine.PrevFormField().has_value());
    EXPECT_EQ(engine.PrevFormField()->id, 1u);
}

// ─── Null model guard ─────────────────────────────────────────────────────────

TEST(NavigationEngineTest, NullModelAllMethodsReturnSafe) {
    NavigationEngine engine(nullptr);
    EXPECT_FALSE(engine.GetFocused().has_value());
    EXPECT_FALSE(engine.Navigate(NavigationDirection::Next).has_value());
    EXPECT_FALSE(engine.Navigate(NavigationDirection::Previous).has_value());
    EXPECT_FALSE(engine.GetParent().has_value());
    EXPECT_FALSE(engine.GetFirstChild().has_value());
    EXPECT_FALSE(engine.GetLastChild().has_value());
    EXPECT_FALSE(engine.NextHeading().has_value());
    EXPECT_FALSE(engine.PrevHeading().has_value());
    EXPECT_TRUE(engine.AllOfRole(AccessRole::Button).empty());
}
