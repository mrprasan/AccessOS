// Tests/Unit/Test_SpeechFormatter.cpp
// Unit tests for SpeechFormatter and SpeechQueue.
// No SAPI or live engine required.

#include <gtest/gtest.h>
#include "Speech/SpeechFormatter.h"
#include "Speech/SpeechQueue.h"
#include "Speech/SpeechPolicy.h"

using namespace AccessOS;

// ─── Helpers ─────────────────────────────────────────────────────────────────

static AccessNode MakeNode(AccessRole role, const std::string& name,
                           const std::string& value = {},
                           AccessState state = AccessState::None)
{
    AccessNode n;
    n.id    = 1;
    n.role  = role;
    n.name  = name;
    n.value = value;
    n.state = state;
    return n;
}

// ─── Section 13 canonical examples ───────────────────────────────────────────

TEST(SpeechFormatterTest, ButtonFormatIsNameCommaRole) {
    // "Submit, button"
    auto node = MakeNode(AccessRole::Button, "Submit");
    auto text = SpeechFormatter::Format(node, SpeechPolicy::Standard());
    EXPECT_EQ(text, "Submit, button");
}

TEST(SpeechFormatterTest, CheckboxCheckedFormat) {
    // "Remember me, checkbox, checked"
    auto node = MakeNode(AccessRole::CheckBox, "Remember me",
                         {}, AccessState::Checked);
    auto text = SpeechFormatter::Format(node, SpeechPolicy::Standard());
    EXPECT_EQ(text, "Remember me, checkbox, checked");
}

TEST(SpeechFormatterTest, CheckboxUncheckedFormat) {
    auto node = MakeNode(AccessRole::CheckBox, "Accept terms");
    auto text = SpeechFormatter::Format(node, SpeechPolicy::Standard());
    EXPECT_EQ(text, "Accept terms, checkbox");
}

TEST(SpeechFormatterTest, HeadingFormatIncludesLevel) {
    // "Accessibility Testing, heading level 2"
    auto node = MakeNode(AccessRole::Heading, "Accessibility Testing");
    node.headingLevel = 2;
    auto text = SpeechFormatter::Format(node, SpeechPolicy::Standard());
    EXPECT_EQ(text, "Accessibility Testing, heading level 2");
}

TEST(SpeechFormatterTest, EditFormatIsNameCommaEdit) {
    // "Username, edit"
    auto node = MakeNode(AccessRole::Edit, "Username");
    node.state = AccessState::MultiLine;
    auto text = SpeechFormatter::Format(node, SpeechPolicy::Standard());
    EXPECT_EQ(text, "Username, edit, multi-line");
}

TEST(SpeechFormatterTest, LinkFormat) {
    auto node = MakeNode(AccessRole::Link, "Learn more");
    auto text = SpeechFormatter::Format(node, SpeechPolicy::Standard());
    EXPECT_EQ(text, "Learn more, link");
}

// ─── Protected field ─────────────────────────────────────────────────────────

TEST(SpeechFormatterTest, PasswordFieldNeverSpeaksValue) {
    auto node = MakeNode(AccessRole::PasswordEdit, "Password", "secret123",
                         AccessState::Protected);
    auto text = SpeechFormatter::Format(node, SpeechPolicy::Standard());
    // Must not contain the actual value
    EXPECT_EQ(text.find("secret123"), std::string::npos);
    EXPECT_NE(text.find("Password"), std::string::npos);
}

// ─── Verbosity levels ─────────────────────────────────────────────────────────

TEST(SpeechFormatterTest, MinimalPolicyOmitsRole) {
    auto node = MakeNode(AccessRole::Button, "Submit");
    auto text = SpeechFormatter::Format(node, SpeechPolicy::Minimal());
    EXPECT_EQ(text, "Submit");
}

TEST(SpeechFormatterTest, MinimalPolicyOmitsState) {
    auto node = MakeNode(AccessRole::CheckBox, "Accept",
                         {}, AccessState::Checked);
    auto text = SpeechFormatter::Format(node, SpeechPolicy::Minimal());
    EXPECT_EQ(text.find("checked"), std::string::npos);
}

TEST(SpeechFormatterTest, DetailedPolicyIncludesDescription) {
    auto node = MakeNode(AccessRole::Button, "Submit");
    node.description = "Submits the form";
    SpeechPolicy policy = SpeechPolicy::Detailed();
    auto text = SpeechFormatter::Format(node, policy);
    EXPECT_NE(text.find("Submits the form"), std::string::npos);
}

// ─── Position ────────────────────────────────────────────────────────────────

TEST(SpeechFormatterTest, PositionAnnouncedWhenPolicyEnabled) {
    auto node = MakeNode(AccessRole::ListItem, "Item");
    node.positionInSet = 3;
    node.setSize       = 10;
    SpeechPolicy policy = SpeechPolicy::Detailed();
    auto text = SpeechFormatter::Format(node, policy);
    EXPECT_NE(text.find("3 of 10"), std::string::npos);
}

TEST(SpeechFormatterTest, PositionNotAnnouncedByDefault) {
    auto node = MakeNode(AccessRole::ListItem, "Item");
    node.positionInSet = 1;
    node.setSize       = 5;
    auto text = SpeechFormatter::Format(node, SpeechPolicy::Standard());
    EXPECT_EQ(text.find("1 of 5"), std::string::npos);
}

// ─── State flags ─────────────────────────────────────────────────────────────

TEST(SpeechFormatterTest, DisabledStateIsAnnounced) {
    auto node = MakeNode(AccessRole::Button, "Submit", {}, AccessState::Disabled);
    auto text = SpeechFormatter::Format(node, SpeechPolicy::Standard());
    EXPECT_NE(text.find("unavailable"), std::string::npos);
}

TEST(SpeechFormatterTest, ExpandedStateIsAnnounced) {
    auto node = MakeNode(AccessRole::TreeItem, "Node", {}, AccessState::Expanded);
    auto text = SpeechFormatter::Format(node, SpeechPolicy::Standard());
    EXPECT_NE(text.find("expanded"), std::string::npos);
}

TEST(SpeechFormatterTest, CollapsedStateIsAnnounced) {
    auto node = MakeNode(AccessRole::TreeItem, "Node", {}, AccessState::Collapsed);
    auto text = SpeechFormatter::Format(node, SpeechPolicy::Standard());
    EXPECT_NE(text.find("collapsed"), std::string::npos);
}

// ─── SpeechQueue tests ────────────────────────────────────────────────────────

TEST(SpeechQueueTest, HighPriorityDequeuedBeforeLow) {
    SpeechQueue q;

    SpeechRequest low;
    low.text     = "low";
    low.priority = SpeechPriority::Low;

    SpeechRequest high;
    high.text     = "high";
    high.priority = SpeechPriority::High;

    q.Enqueue(low);
    q.Enqueue(high);

    auto first = q.Dequeue(std::chrono::milliseconds(100));
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->text, "high");

    auto second = q.Dequeue(std::chrono::milliseconds(100));
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(second->text, "low");
}

TEST(SpeechQueueTest, CancelUpToRemovesLowerPriority) {
    SpeechQueue q;

    SpeechRequest n;
    n.text = "normal"; n.priority = SpeechPriority::Normal;
    SpeechRequest c;
    c.text = "critical"; c.priority = SpeechPriority::Critical;

    q.Enqueue(n);
    q.Enqueue(c);

    q.CancelUpTo(SpeechPriority::Normal);   // removes Normal and below

    EXPECT_EQ(q.Size(), 1u);
    auto item = q.Dequeue(std::chrono::milliseconds(100));
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item->text, "critical");
}

TEST(SpeechQueueTest, CancelAllEmptiesQueue) {
    SpeechQueue q;
    for (int i = 0; i < 5; ++i) {
        SpeechRequest r;
        r.text     = "t";
        r.priority = SpeechPriority::Normal;
        q.Enqueue(r);
    }
    q.CancelAll();
    EXPECT_EQ(q.Size(), 0u);
}

TEST(SpeechQueueTest, CancelPreviousRemovesLowerOnEnqueue) {
    SpeechQueue q;

    SpeechRequest low;
    low.text = "old"; low.priority = SpeechPriority::Low;
    q.Enqueue(low);

    SpeechRequest nav;
    nav.text           = "new nav";
    nav.priority       = SpeechPriority::Normal;
    nav.cancelPrevious = true;   // removes Low items
    q.Enqueue(nav);

    // Only the new nav request remains
    EXPECT_EQ(q.Size(), 1u);
    auto item = q.Dequeue(std::chrono::milliseconds(100));
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item->text, "new nav");
}

TEST(SpeechQueueTest, StopWakesBlockedDequeue) {
    SpeechQueue q;
    bool woken = false;

    std::thread t([&] {
        auto r = q.Dequeue(std::chrono::milliseconds(5000));
        woken = !r.has_value();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    q.Stop();
    t.join();
    EXPECT_TRUE(woken);
}
