// Tests/Unit/Test_FocusManager.cpp
// Unit tests for FocusManager — duplicate suppression, history, observers.
// No live UIA required.

#include <gtest/gtest.h>
#include "Focus/FocusManager.h"
#include "Semantic/SemanticModel.h"
#include "Events/IEventListener.h"

#include <vector>
#include <string>

using namespace AccessOS;

// ─── Helpers ─────────────────────────────────────────────────────────────────

static AccessEvent MakeFocusEvent(uint64_t id, AccessRole role,
                                  const std::string& name,
                                  uint32_t pid = 1) {
    AccessEvent evt;
    evt.type               = AccessEventType::FocusChanged;
    evt.element.id         = id;
    evt.element.role       = role;
    evt.element.name       = name;
    evt.element.processId  = pid;
    evt.element.isValid    = true;
    evt.timestampMs        = 0;
    return evt;
}

// ─── Basic focus tracking ────────────────────────────────────────────────────

TEST(FocusManagerTest, InitialStateHasNoFocus) {
    SemanticModel model;
    FocusManager fm;
    fm.Initialize(&model);

    EXPECT_FALSE(fm.GetCurrentFocus().has_value());
    EXPECT_FALSE(fm.GetPreviousFocus().has_value());
    EXPECT_TRUE(fm.GetHistory().empty());
}

TEST(FocusManagerTest, FirstFocusEventIsRecorded) {
    SemanticModel model;
    FocusManager fm;
    fm.Initialize(&model);

    model.OnEvent(MakeFocusEvent(1, AccessRole::Button, "Submit"));

    auto focus = fm.GetCurrentFocus();
    ASSERT_TRUE(focus.has_value());
    EXPECT_EQ(focus->id,   1u);
    EXPECT_EQ(focus->name, "Submit");
    EXPECT_EQ(focus->role, AccessRole::Button);
}

TEST(FocusManagerTest, FocusChangeCountIncrements) {
    SemanticModel model;
    FocusManager fm;
    fm.Initialize(&model);

    model.OnEvent(MakeFocusEvent(1, AccessRole::Button, "A"));
    model.OnEvent(MakeFocusEvent(2, AccessRole::Edit,   "B"));

    EXPECT_EQ(fm.FocusChangeCount(), 2u);
}

// ─── Duplicate suppression ────────────────────────────────────────────────────

TEST(FocusManagerTest, DuplicateFocusEventIsSuppressed) {
    SemanticModel model;
    FocusManager fm;
    fm.Initialize(&model);

    model.OnEvent(MakeFocusEvent(1, AccessRole::Button, "OK"));
    model.OnEvent(MakeFocusEvent(1, AccessRole::Button, "OK")); // duplicate

    EXPECT_EQ(fm.FocusChangeCount(),    1u);
    EXPECT_EQ(fm.DuplicatesSuppressed(), 1u);
}

TEST(FocusManagerTest, DifferentElementIsNotSuppressed) {
    SemanticModel model;
    FocusManager fm;
    fm.Initialize(&model);

    model.OnEvent(MakeFocusEvent(1, AccessRole::Button, "OK"));
    model.OnEvent(MakeFocusEvent(2, AccessRole::Button, "Cancel"));

    EXPECT_EQ(fm.FocusChangeCount(),    2u);
    EXPECT_EQ(fm.DuplicatesSuppressed(), 0u);
}

TEST(FocusManagerTest, MultipleDuplicatesAllSuppressed) {
    SemanticModel model;
    FocusManager fm;
    fm.Initialize(&model);

    model.OnEvent(MakeFocusEvent(5, AccessRole::Edit, "Search"));
    model.OnEvent(MakeFocusEvent(5, AccessRole::Edit, "Search"));
    model.OnEvent(MakeFocusEvent(5, AccessRole::Edit, "Search"));
    model.OnEvent(MakeFocusEvent(5, AccessRole::Edit, "Search"));

    EXPECT_EQ(fm.FocusChangeCount(),    1u);
    EXPECT_EQ(fm.DuplicatesSuppressed(), 3u);
}

// ─── Previous focus ───────────────────────────────────────────────────────────

TEST(FocusManagerTest, PreviousFocusIsRetainedAfterChange) {
    SemanticModel model;
    FocusManager fm;
    fm.Initialize(&model);

    model.OnEvent(MakeFocusEvent(1, AccessRole::Button, "First"));
    model.OnEvent(MakeFocusEvent(2, AccessRole::Edit,   "Second"));

    auto prev = fm.GetPreviousFocus();
    ASSERT_TRUE(prev.has_value());
    EXPECT_EQ(prev->name, "First");
    EXPECT_EQ(prev->id,   1u);
}

TEST(FocusManagerTest, PreviousFocusIsNulloptAfterOnlyOneChange) {
    SemanticModel model;
    FocusManager fm;
    fm.Initialize(&model);

    model.OnEvent(MakeFocusEvent(1, AccessRole::Button, "Only"));

    EXPECT_FALSE(fm.GetPreviousFocus().has_value());
}

// ─── History ─────────────────────────────────────────────────────────────────

TEST(FocusManagerTest, HistoryIsNewestFirst) {
    SemanticModel model;
    FocusManager fm;
    fm.Initialize(&model);

    model.OnEvent(MakeFocusEvent(1, AccessRole::Button, "A"));
    model.OnEvent(MakeFocusEvent(2, AccessRole::Button, "B"));
    model.OnEvent(MakeFocusEvent(3, AccessRole::Button, "C"));

    auto history = fm.GetHistory(3);
    ASSERT_EQ(history.size(), 3u);
    EXPECT_EQ(history[0].name, "C");   // newest
    EXPECT_EQ(history[1].name, "B");
    EXPECT_EQ(history[2].name, "A");   // oldest
}

TEST(FocusManagerTest, HistoryRespectesMaxCountParameter) {
    SemanticModel model;
    FocusManager fm;
    fm.Initialize(&model);

    for (int i = 1; i <= 10; ++i) {
        model.OnEvent(MakeFocusEvent(
            static_cast<uint64_t>(i), AccessRole::Button,
            "btn" + std::to_string(i)));
    }

    auto history = fm.GetHistory(3);
    EXPECT_EQ(history.size(), 3u);
}

TEST(FocusManagerTest, HistoryDoesNotExceedMaxSize) {
    SemanticModel model;
    FocusManager fm;
    fm.Initialize(&model);

    // Push more than kFocusHistoryMaxSize unique elements
    for (uint64_t i = 1; i <= kFocusHistoryMaxSize + 10; ++i) {
        model.OnEvent(MakeFocusEvent(i, AccessRole::Button, "b" + std::to_string(i)));
    }

    auto history = fm.GetHistory(kFocusHistoryMaxSize + 10);
    EXPECT_LE(history.size(), kFocusHistoryMaxSize);
}

// ─── Observer notification ───────────────────────────────────────────────────

TEST(FocusManagerTest, ObserverIsCalledOnGenuineChange) {
    SemanticModel model;
    FocusManager fm;
    fm.Initialize(&model);

    std::vector<std::string> received;
    fm.AddObserver([&](const AccessNode& node) {
        received.push_back(node.name);
    });

    model.OnEvent(MakeFocusEvent(1, AccessRole::Button, "Alpha"));
    model.OnEvent(MakeFocusEvent(2, AccessRole::Edit,   "Beta"));

    ASSERT_EQ(received.size(), 2u);
    EXPECT_EQ(received[0], "Alpha");
    EXPECT_EQ(received[1], "Beta");
}

TEST(FocusManagerTest, ObserverNotCalledOnDuplicate) {
    SemanticModel model;
    FocusManager fm;
    fm.Initialize(&model);

    int callCount = 0;
    fm.AddObserver([&](const AccessNode&) { ++callCount; });

    model.OnEvent(MakeFocusEvent(1, AccessRole::Button, "X"));
    model.OnEvent(MakeFocusEvent(1, AccessRole::Button, "X")); // duplicate

    EXPECT_EQ(callCount, 1);
}

TEST(FocusManagerTest, MultipleObserversAllReceiveNotification) {
    SemanticModel model;
    FocusManager fm;
    fm.Initialize(&model);

    int count1 = 0, count2 = 0;
    fm.AddObserver([&](const AccessNode&) { ++count1; });
    fm.AddObserver([&](const AccessNode&) { ++count2; });

    model.OnEvent(MakeFocusEvent(10, AccessRole::Edit, "Field"));

    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);
}
