// Tests/Unit/Test_ShortcutManager.cpp
// Unit tests for ShortcutManager — bind, lookup, conflict, rebind, unbind.
// No live UIA, no keyboard, no speech required.

#include <gtest/gtest.h>
#include "Commands/ShortcutManager.h"

using namespace AccessOS;

// ─── Helpers ─────────────────────────────────────────────────────────────────

static KeyStroke KS(uint32_t vk, KeyModifier mods = KeyModifier::None) {
    return KeyStroke{ vk, mods };
}

// ─── Bind / Lookup ────────────────────────────────────────────────────────────

TEST(ShortcutManagerTest, BindAndLookup) {
    ShortcutManager mgr;
    EXPECT_TRUE(mgr.Bind(KS('H', KeyModifier::Ctrl), "nav.next"));
    auto result = mgr.Lookup(KS('H', KeyModifier::Ctrl));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "nav.next");
}

TEST(ShortcutManagerTest, LookupUnboundReturnsNullopt) {
    ShortcutManager mgr;
    EXPECT_FALSE(mgr.Lookup(KS('Z')).has_value());
}

TEST(ShortcutManagerTest, CountAfterBind) {
    ShortcutManager mgr;
    EXPECT_EQ(mgr.Count(), 0u);
    mgr.Bind(KS('A'), "cmd.a");
    mgr.Bind(KS('B'), "cmd.b");
    EXPECT_EQ(mgr.Count(), 2u);
}

// ─── Conflict detection ───────────────────────────────────────────────────────

TEST(ShortcutManagerTest, DuplicateBindReturnsFalse) {
    ShortcutManager mgr;
    EXPECT_TRUE(mgr.Bind(KS('X'), "first"));
    EXPECT_FALSE(mgr.Bind(KS('X'), "second"));  // Conflict.
    // Original binding survives.
    EXPECT_EQ(*mgr.Lookup(KS('X')), "first");
}

TEST(ShortcutManagerTest, HasConflictTrue) {
    ShortcutManager mgr;
    mgr.Bind(KS('Q'), "cmd.q");
    EXPECT_TRUE(mgr.HasConflict(KS('Q')));
}

TEST(ShortcutManagerTest, HasConflictFalse) {
    ShortcutManager mgr;
    EXPECT_FALSE(mgr.HasConflict(KS('Q')));
}

// ─── Unbind ───────────────────────────────────────────────────────────────────

TEST(ShortcutManagerTest, UnbindExistingReturnsTrue) {
    ShortcutManager mgr;
    mgr.Bind(KS('D'), "del.cmd");
    EXPECT_TRUE(mgr.Unbind(KS('D')));
    EXPECT_FALSE(mgr.Lookup(KS('D')).has_value());
    EXPECT_EQ(mgr.Count(), 0u);
}

TEST(ShortcutManagerTest, UnbindNonExistentReturnsFalse) {
    ShortcutManager mgr;
    EXPECT_FALSE(mgr.Unbind(KS('Z')));
}

// ─── Rebind ───────────────────────────────────────────────────────────────────

TEST(ShortcutManagerTest, RebindUpdatesCommandId) {
    ShortcutManager mgr;
    mgr.Bind(KS('R'), "old.cmd");
    EXPECT_TRUE(mgr.Rebind(KS('R'), "new.cmd"));
    EXPECT_EQ(*mgr.Lookup(KS('R')), "new.cmd");
}

TEST(ShortcutManagerTest, RebindNonExistentReturnsFalse) {
    ShortcutManager mgr;
    EXPECT_FALSE(mgr.Rebind(KS('Z'), "any.cmd"));
}

// ─── AllBindings ─────────────────────────────────────────────────────────────

TEST(ShortcutManagerTest, AllBindingsContainsAll) {
    ShortcutManager mgr;
    mgr.Bind(KS('A'), "cmd.a");
    mgr.Bind(KS('B'), "cmd.b");
    const auto all = mgr.AllBindings();
    EXPECT_EQ(all.size(), 2u);
}

TEST(ShortcutManagerTest, AllBindingsEmptyWhenNone) {
    ShortcutManager mgr;
    EXPECT_TRUE(mgr.AllBindings().empty());
}

// ─── Modifier flag combinations ───────────────────────────────────────────────

TEST(ShortcutManagerTest, CtrlShiftCombo) {
    ShortcutManager mgr;
    const KeyModifier mods = KeyModifier::Ctrl | KeyModifier::Shift;
    mgr.Bind(KS('F', mods), "find.cmd");
    auto result = mgr.Lookup(KS('F', mods));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "find.cmd");
    // Plain 'F' is not the same shortcut.
    EXPECT_FALSE(mgr.Lookup(KS('F')).has_value());
}

TEST(ShortcutManagerTest, HasModifierHelper) {
    const KeyModifier mods = KeyModifier::Ctrl | KeyModifier::Alt;
    EXPECT_TRUE(HasModifier(mods, KeyModifier::Ctrl));
    EXPECT_TRUE(HasModifier(mods, KeyModifier::Alt));
    EXPECT_FALSE(HasModifier(mods, KeyModifier::Shift));
    EXPECT_FALSE(HasModifier(mods, KeyModifier::Win));
}
