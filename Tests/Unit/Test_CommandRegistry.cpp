// AccessOS/Tests/Unit/Test_CommandRegistry.cpp
//
// Unit tests for CommandRegistry.
//
// Coverage:
//   - RegisterAll registers all 15 built-in command IDs
//   - All registered commands are callable (Execute does not crash)
//   - Commands with null subsystem pointers are safe (no crash)
//   - BindDefaults binds the expected number of shortcuts
//   - BindDefaults binds specific expected key combinations
//   - CmdId constant values are non-empty strings
//   - No duplicate shortcuts after BindDefaults

#include <gtest/gtest.h>
#include "Commands/CommandRegistry.h"
#include "Commands/CommandManager.h"
#include "Commands/ShortcutManager.h"

#include <windows.h>

using namespace AccessOS;

// ─── CmdId constants ──────────────────────────────────────────────────────────

TEST(CmdId, AllConstantsNonEmpty) {
    EXPECT_NE(std::string(CmdId::StopSpeech),     "");
    EXPECT_NE(std::string(CmdId::ReadFocused),    "");
    EXPECT_NE(std::string(CmdId::ReadTitle),      "");
    EXPECT_NE(std::string(CmdId::NavNext),        "");
    EXPECT_NE(std::string(CmdId::NavPrev),        "");
    EXPECT_NE(std::string(CmdId::NavNextHeading), "");
    EXPECT_NE(std::string(CmdId::NavPrevHeading), "");
    EXPECT_NE(std::string(CmdId::NavNextLink),    "");
    EXPECT_NE(std::string(CmdId::NavPrevLink),    "");
    EXPECT_NE(std::string(CmdId::NavNextButton),  "");
    EXPECT_NE(std::string(CmdId::NavPrevButton),  "");
    EXPECT_NE(std::string(CmdId::NavNextForm),    "");
    EXPECT_NE(std::string(CmdId::NavPrevForm),    "");
    EXPECT_NE(std::string(CmdId::NavParent),      "");
    EXPECT_NE(std::string(CmdId::NavFirstChild),  "");
}

TEST(CmdId, AllConstantsDistinct) {
    const char* ids[] = {
        CmdId::StopSpeech, CmdId::ReadFocused, CmdId::ReadTitle,
        CmdId::NavNext, CmdId::NavPrev,
        CmdId::NavNextHeading, CmdId::NavPrevHeading,
        CmdId::NavNextLink, CmdId::NavPrevLink,
        CmdId::NavNextButton, CmdId::NavPrevButton,
        CmdId::NavNextForm, CmdId::NavPrevForm,
        CmdId::NavParent, CmdId::NavFirstChild,
    };
    for (size_t i = 0; i < std::size(ids); ++i) {
        for (size_t j = i + 1; j < std::size(ids); ++j) {
            EXPECT_STRNE(ids[i], ids[j])
                << "Duplicate CmdId at indices " << i << " and " << j;
        }
    }
}

// ─── RegisterAll — null pointers ─────────────────────────────────────────────

TEST(CommandRegistry_RegisterAll, RegistersWithNullPointers) {
    CommandManager cm;
    // All subsystem pointers null — must not crash.
    EXPECT_NO_THROW(
        CommandRegistry::RegisterAll(cm, nullptr, nullptr, nullptr, nullptr));
}

TEST(CommandRegistry_RegisterAll, RegistersAllFifteenCommands) {
    CommandManager cm;
    CommandRegistry::RegisterAll(cm, nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(cm.Count(), 15u);
}

TEST(CommandRegistry_RegisterAll, AllCommandIdsPresent) {
    CommandManager cm;
    CommandRegistry::RegisterAll(cm, nullptr, nullptr, nullptr, nullptr);

    for (const char* id : {
        CmdId::StopSpeech, CmdId::ReadFocused, CmdId::ReadTitle,
        CmdId::NavNext, CmdId::NavPrev,
        CmdId::NavNextHeading, CmdId::NavPrevHeading,
        CmdId::NavNextLink, CmdId::NavPrevLink,
        CmdId::NavNextButton, CmdId::NavPrevButton,
        CmdId::NavNextForm, CmdId::NavPrevForm,
        CmdId::NavParent, CmdId::NavFirstChild,
    }) {
        EXPECT_NE(cm.Get(id), nullptr) << "Command not found: " << id;
    }
}

TEST(CommandRegistry_RegisterAll, ExecuteWithNullSubsystemsDoesNotCrash) {
    CommandManager cm;
    CommandRegistry::RegisterAll(cm, nullptr, nullptr, nullptr, nullptr);

    // All commands should be safe to Execute with null subsystem pointers.
    for (const char* id : {
        CmdId::StopSpeech, CmdId::ReadFocused, CmdId::ReadTitle,
        CmdId::NavNext, CmdId::NavPrev,
        CmdId::NavNextHeading, CmdId::NavPrevHeading,
        CmdId::NavNextLink, CmdId::NavPrevLink,
        CmdId::NavNextButton, CmdId::NavPrevButton,
        CmdId::NavNextForm, CmdId::NavPrevForm,
        CmdId::NavParent, CmdId::NavFirstChild,
    }) {
        EXPECT_NO_THROW(cm.Execute(id)) << "Execute crashed for: " << id;
    }
}

TEST(CommandRegistry_RegisterAll, CommandDisplayNamesNonEmpty) {
    CommandManager cm;
    CommandRegistry::RegisterAll(cm, nullptr, nullptr, nullptr, nullptr);

    for (const char* id : {
        CmdId::StopSpeech, CmdId::ReadFocused, CmdId::ReadTitle,
        CmdId::NavNext, CmdId::NavPrev,
    }) {
        ICommand* cmd = cm.Get(id);
        ASSERT_NE(cmd, nullptr);
        EXPECT_STRNE(cmd->DisplayName(), "");
    }
}

// ─── BindDefaults ─────────────────────────────────────────────────────────────

TEST(CommandRegistry_BindDefaults, BindsExpectedCount) {
    ShortcutManager sm;
    CommandRegistry::BindDefaults(sm);
    // 12 nav + 3 reader = 15 bindings
    EXPECT_EQ(sm.Count(), 15u);
}

TEST(CommandRegistry_BindDefaults, NoDuplicates) {
    ShortcutManager sm;
    CommandRegistry::BindDefaults(sm);
    // If there were duplicates, Count would be less than 15.
    EXPECT_EQ(sm.Count(), 15u);
}

TEST(CommandRegistry_BindDefaults, CapsLockSpaceReadsNav) {
    ShortcutManager sm;
    CommandRegistry::BindDefaults(sm);
    KeyStroke ks{ VK_SPACE, KeyModifier::Win };
    auto id = sm.Lookup(ks);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(*id, std::string(CmdId::ReadFocused));
}

TEST(CommandRegistry_BindDefaults, CapsLockRightArrowNavNext) {
    ShortcutManager sm;
    CommandRegistry::BindDefaults(sm);
    KeyStroke ks{ VK_RIGHT, KeyModifier::Win };
    auto id = sm.Lookup(ks);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(*id, std::string(CmdId::NavNext));
}

TEST(CommandRegistry_BindDefaults, CapsLockLeftArrowNavPrev) {
    ShortcutManager sm;
    CommandRegistry::BindDefaults(sm);
    KeyStroke ks{ VK_LEFT, KeyModifier::Win };
    auto id = sm.Lookup(ks);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(*id, std::string(CmdId::NavPrev));
}

TEST(CommandRegistry_BindDefaults, CapsLockHNextHeading) {
    ShortcutManager sm;
    CommandRegistry::BindDefaults(sm);
    KeyStroke ks{ 'H', KeyModifier::Win };
    auto id = sm.Lookup(ks);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(*id, std::string(CmdId::NavNextHeading));
}

TEST(CommandRegistry_BindDefaults, CapsLockShiftHPrevHeading) {
    ShortcutManager sm;
    CommandRegistry::BindDefaults(sm);
    KeyStroke ks{ 'H', KeyModifier::Win | KeyModifier::Shift };
    auto id = sm.Lookup(ks);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(*id, std::string(CmdId::NavPrevHeading));
}

TEST(CommandRegistry_BindDefaults, CapsLockKNextLink) {
    ShortcutManager sm;
    CommandRegistry::BindDefaults(sm);
    KeyStroke ks{ 'K', KeyModifier::Win };
    auto id = sm.Lookup(ks);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(*id, std::string(CmdId::NavNextLink));
}

TEST(CommandRegistry_BindDefaults, CapsLockBNextButton) {
    ShortcutManager sm;
    CommandRegistry::BindDefaults(sm);
    KeyStroke ks{ 'B', KeyModifier::Win };
    auto id = sm.Lookup(ks);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(*id, std::string(CmdId::NavNextButton));
}

TEST(CommandRegistry_BindDefaults, CapsLockENextFormField) {
    ShortcutManager sm;
    CommandRegistry::BindDefaults(sm);
    KeyStroke ks{ 'E', KeyModifier::Win };
    auto id = sm.Lookup(ks);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(*id, std::string(CmdId::NavNextForm));
}

TEST(CommandRegistry_BindDefaults, CapsLockCtrlSStopSpeech) {
    ShortcutManager sm;
    CommandRegistry::BindDefaults(sm);
    KeyStroke ks{ 'S', KeyModifier::Win | KeyModifier::Ctrl };
    auto id = sm.Lookup(ks);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(*id, std::string(CmdId::StopSpeech));
}

TEST(CommandRegistry_BindDefaults, CapsLockTReadTitle) {
    ShortcutManager sm;
    CommandRegistry::BindDefaults(sm);
    KeyStroke ks{ 'T', KeyModifier::Win };
    auto id = sm.Lookup(ks);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(*id, std::string(CmdId::ReadTitle));
}

TEST(CommandRegistry_BindDefaults, CapsLockUpNavParent) {
    ShortcutManager sm;
    CommandRegistry::BindDefaults(sm);
    KeyStroke ks{ VK_UP, KeyModifier::Win };
    auto id = sm.Lookup(ks);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(*id, std::string(CmdId::NavParent));
}

TEST(CommandRegistry_BindDefaults, CapsLockDownNavFirstChild) {
    ShortcutManager sm;
    CommandRegistry::BindDefaults(sm);
    KeyStroke ks{ VK_DOWN, KeyModifier::Win };
    auto id = sm.Lookup(ks);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(*id, std::string(CmdId::NavFirstChild));
}

TEST(CommandRegistry_BindDefaults, UnboundKeyReturnsNullopt) {
    ShortcutManager sm;
    CommandRegistry::BindDefaults(sm);
    // VK_F12 with no modifiers — not bound.
    KeyStroke ks{ VK_F12, KeyModifier::None };
    EXPECT_FALSE(sm.Lookup(ks).has_value());
}
