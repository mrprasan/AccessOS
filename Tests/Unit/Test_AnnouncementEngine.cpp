// AccessOS/Tests/Unit/Test_AnnouncementEngine.cpp
//
// Unit tests for Announcement, AnnouncementPart, and AnnouncementEngine.
// Coverage: Announcement helpers, all BuildFocusAnnouncement rules,
//           BuildPropertyAnnouncement, BuildAlertAnnouncement,
//           context suppression, dialog R9, verbosity levels.

#include <gtest/gtest.h>
#include "Announcement/AnnouncementEngine.h"
#include "Semantic/AccessNode.h"
#include "Context/AppContext.h"
#include "Speech/SpeechPolicy.h"

using namespace AccessOS;

// ── Helpers ───────────────────────────────────────────────────────────────────

static AccessNode MakeNode(AccessRole role,
                            const std::string& name,
                            const std::string& value = "",
                            const std::string& description = "",
                            AccessState state = AccessState::None)
{
    AccessNode n;
    n.role        = role;
    n.name        = name;
    n.value       = value;
    n.description = description;
    n.state       = state;
    n.isValid     = true;
    return n;
}

static AppContext MakeCtx(AppContextType type = AppContextType::DesktopApp,
                           bool isModal = false,
                           const std::string& windowTitle = "")
{
    AppContext ctx;
    ctx.type        = type;
    ctx.isModal     = isModal;
    ctx.windowTitle = windowTitle;
    return ctx;
}

// ─── Announcement helpers ─────────────────────────────────────────────────────

TEST(Announcement, IsEmptyWhenNoParts) {
    Announcement ann;
    EXPECT_TRUE(ann.IsEmpty());
}

TEST(Announcement, IsNotEmptyAfterAdd) {
    Announcement ann;
    ann.Add(AnnouncementPartKind::Name, "Submit");
    EXPECT_FALSE(ann.IsEmpty());
}

TEST(Announcement, AddIgnoresEmptyText) {
    Announcement ann;
    ann.Add(AnnouncementPartKind::Name, "");
    EXPECT_TRUE(ann.IsEmpty());
    EXPECT_EQ(ann.parts.size(), 0u);
}

TEST(Announcement, FlattenJoinsWithCommaSpace) {
    Announcement ann;
    ann.Add(AnnouncementPartKind::Name, "Submit");
    ann.Add(AnnouncementPartKind::Role, "button");
    EXPECT_EQ(ann.Flatten(), "Submit, button");
}

TEST(Announcement, FlattenSkipsEmptyParts) {
    Announcement ann;
    ann.Add(AnnouncementPartKind::Name, "OK");
    ann.parts.push_back({ AnnouncementPartKind::State, "" });  // empty state
    ann.Add(AnnouncementPartKind::Role, "button");
    EXPECT_EQ(ann.Flatten(), "OK, button");
}

TEST(Announcement, FlattenReturnsEmptyStringWhenAllEmpty) {
    Announcement ann;
    EXPECT_EQ(ann.Flatten(), "");
}

TEST(Announcement, AddTextAppendsCustomPart) {
    Announcement ann;
    ann.AddText("hello");
    ASSERT_EQ(ann.parts.size(), 1u);
    EXPECT_EQ(ann.parts[0].kind, AnnouncementPartKind::Custom);
    EXPECT_EQ(ann.parts[0].text, "hello");
}

TEST(Announcement, DefaultPriorityIsNormal) {
    Announcement ann;
    EXPECT_EQ(ann.priority, SpeechPriority::Normal);
}

// ─── BuildFocusAnnouncement — R1 invalid node ─────────────────────────────────

TEST(AnnouncementEngine_Focus, InvalidNodeProducesEmptyAnnouncement) {
    AccessNode n = MakeNode(AccessRole::Button, "OK");
    n.isValid = false;
    AppContext ctx = MakeCtx();
    SpeechPolicy pol;
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    EXPECT_TRUE(ann.IsEmpty());
}

// ─── BuildFocusAnnouncement — R2 name ─────────────────────────────────────────

TEST(AnnouncementEngine_Focus, NameIsFirstPart) {
    AccessNode n = MakeNode(AccessRole::Button, "Submit");
    AppContext ctx = MakeCtx();
    SpeechPolicy pol;
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    ASSERT_FALSE(ann.parts.empty());
    EXPECT_EQ(ann.parts[0].kind, AnnouncementPartKind::Name);
    EXPECT_EQ(ann.parts[0].text, "Submit");
}

TEST(AnnouncementEngine_Focus, EmptyNameProducesNoPart) {
    AccessNode n = MakeNode(AccessRole::Button, "");
    AppContext ctx = MakeCtx();
    SpeechPolicy pol;
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    for (const auto& p : ann.parts) {
        EXPECT_NE(p.kind, AnnouncementPartKind::Name);
    }
}

// ─── BuildFocusAnnouncement — R3 role ─────────────────────────────────────────

TEST(AnnouncementEngine_Focus, RoleIsAnnouncedAfterName) {
    AccessNode n = MakeNode(AccessRole::Button, "Submit");
    AppContext ctx = MakeCtx();
    SpeechPolicy pol;
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    bool foundRole = false;
    for (const auto& p : ann.parts) {
        if (p.kind == AnnouncementPartKind::Role) {
            EXPECT_EQ(p.text, "button");
            foundRole = true;
        }
    }
    EXPECT_TRUE(foundRole);
}

TEST(AnnouncementEngine_Focus, RoleSuppressedAtMinimalVerbosity) {
    AccessNode n = MakeNode(AccessRole::Button, "Cancel");
    AppContext ctx = MakeCtx();
    SpeechPolicy pol = SpeechPolicy::Minimal();
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    for (const auto& p : ann.parts) {
        EXPECT_NE(p.kind, AnnouncementPartKind::Role);
    }
}

TEST(AnnouncementEngine_Focus, RoleSuppressedWhenAnnounceRoleFalse) {
    AccessNode n = MakeNode(AccessRole::CheckBox, "Remember me");
    AppContext ctx = MakeCtx();
    SpeechPolicy pol;
    pol.announceRole = false;
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    for (const auto& p : ann.parts) {
        EXPECT_NE(p.kind, AnnouncementPartKind::Role);
    }
}

TEST(AnnouncementEngine_Focus, HeadingRoleIncludesLevel) {
    AccessNode n = MakeNode(AccessRole::Heading, "Introduction");
    n.headingLevel = 2;
    AppContext ctx = MakeCtx();
    SpeechPolicy pol;
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    bool found = false;
    for (const auto& p : ann.parts) {
        if (p.kind == AnnouncementPartKind::Role) {
            EXPECT_EQ(p.text, "heading level 2");
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST(AnnouncementEngine_Focus, StaticTextRoleIsEmpty_NotAnnounced) {
    AccessNode n = MakeNode(AccessRole::StaticText, "Hello world");
    AppContext ctx = MakeCtx();
    SpeechPolicy pol;
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    for (const auto& p : ann.parts) {
        EXPECT_NE(p.kind, AnnouncementPartKind::Role);
    }
    // Name should still be there.
    bool foundName = false;
    for (const auto& p : ann.parts) {
        if (p.kind == AnnouncementPartKind::Name) foundName = true;
    }
    EXPECT_TRUE(foundName);
}

// ─── R8: Context suppression ──────────────────────────────────────────────────

TEST(AnnouncementEngine_Focus, TerminalContextSuppressesPaneRole) {
    AccessNode n = MakeNode(AccessRole::Pane, "Terminal area");
    AppContext ctx = MakeCtx(AppContextType::Terminal);
    SpeechPolicy pol;
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    for (const auto& p : ann.parts) {
        EXPECT_NE(p.kind, AnnouncementPartKind::Role);
    }
}

TEST(AnnouncementEngine_Focus, TerminalContextSuppressesWindowRole) {
    AccessNode n = MakeNode(AccessRole::Window, "Windows Terminal");
    AppContext ctx = MakeCtx(AppContextType::Terminal);
    SpeechPolicy pol;
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    for (const auto& p : ann.parts) {
        EXPECT_NE(p.kind, AnnouncementPartKind::Role);
    }
}

TEST(AnnouncementEngine_Focus, TerminalContextDoesNotSuppressButtonRole) {
    AccessNode n = MakeNode(AccessRole::Button, "Close");
    AppContext ctx = MakeCtx(AppContextType::Terminal);
    SpeechPolicy pol;
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    bool found = false;
    for (const auto& p : ann.parts) {
        if (p.kind == AnnouncementPartKind::Role) found = true;
    }
    EXPECT_TRUE(found);
}

TEST(AnnouncementEngine_Focus, CodeEditorSuppressesPaneRole) {
    AccessNode n = MakeNode(AccessRole::Pane, "Editor");
    AppContext ctx = MakeCtx(AppContextType::CodeEditor);
    SpeechPolicy pol;
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    for (const auto& p : ann.parts) {
        EXPECT_NE(p.kind, AnnouncementPartKind::Role);
    }
}

TEST(AnnouncementEngine_Focus, BrowserContextDoesNotSuppressButtonRole) {
    AccessNode n = MakeNode(AccessRole::Button, "Search");
    AppContext ctx = MakeCtx(AppContextType::Browser);
    SpeechPolicy pol;
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    bool found = false;
    for (const auto& p : ann.parts) {
        if (p.kind == AnnouncementPartKind::Role) found = true;
    }
    EXPECT_TRUE(found);
}

// ─── R4: State ───────────────────────────────────────────────────────────────

TEST(AnnouncementEngine_Focus, CheckedStateAnnounced) {
    AccessNode n = MakeNode(AccessRole::CheckBox, "Bold",
                             "", "", AccessState::Checked);
    AppContext ctx = MakeCtx();
    SpeechPolicy pol;
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    bool found = false;
    for (const auto& p : ann.parts) {
        if (p.kind == AnnouncementPartKind::State &&
            p.text.find("checked") != std::string::npos) found = true;
    }
    EXPECT_TRUE(found);
}

TEST(AnnouncementEngine_Focus, DisabledStateAnnounced) {
    AccessNode n = MakeNode(AccessRole::Button, "Save",
                             "", "", AccessState::Disabled);
    AppContext ctx = MakeCtx();
    SpeechPolicy pol;
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    bool found = false;
    for (const auto& p : ann.parts) {
        if (p.kind == AnnouncementPartKind::State &&
            p.text.find("unavailable") != std::string::npos) found = true;
    }
    EXPECT_TRUE(found);
}

TEST(AnnouncementEngine_Focus, StateSuppressedAtMinimalVerbosity) {
    AccessNode n = MakeNode(AccessRole::CheckBox, "Bold",
                             "", "", AccessState::Checked);
    AppContext ctx = MakeCtx();
    SpeechPolicy pol = SpeechPolicy::Minimal();
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    for (const auto& p : ann.parts) {
        EXPECT_NE(p.kind, AnnouncementPartKind::State);
    }
}

// ─── R5: Value ───────────────────────────────────────────────────────────────

TEST(AnnouncementEngine_Focus, ValueAnnouncedForSlider) {
    AccessNode n = MakeNode(AccessRole::Slider, "Volume", "75%");
    AppContext ctx = MakeCtx();
    SpeechPolicy pol;
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    bool found = false;
    for (const auto& p : ann.parts) {
        if (p.kind == AnnouncementPartKind::Value) found = true;
    }
    EXPECT_TRUE(found);
}

TEST(AnnouncementEngine_Focus, PasswordValueNeverAnnounced) {
    AccessNode n = MakeNode(AccessRole::PasswordEdit, "Password", "secret123");
    n.state = AccessState::Protected;
    AppContext ctx = MakeCtx();
    SpeechPolicy pol;
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    for (const auto& p : ann.parts) {
        if (p.kind == AnnouncementPartKind::Value) {
            EXPECT_NE(p.text, "secret123");
        }
    }
}

TEST(AnnouncementEngine_Focus, ValueNotDuplicatedWhenEqualsName) {
    AccessNode n = MakeNode(AccessRole::Edit, "hello", "hello");
    AppContext ctx = MakeCtx();
    SpeechPolicy pol;
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    int valueCount = 0;
    for (const auto& p : ann.parts) {
        if (p.kind == AnnouncementPartKind::Value) ++valueCount;
    }
    EXPECT_EQ(valueCount, 0);  // suppressed because equals name
}

// ─── R6: Position ────────────────────────────────────────────────────────────

TEST(AnnouncementEngine_Focus, PositionAnnouncedWhenPolicyEnabled) {
    AccessNode n = MakeNode(AccessRole::ListItem, "Apple");
    n.positionInSet = 2;
    n.setSize = 5;
    AppContext ctx = MakeCtx();
    SpeechPolicy pol;
    pol.announcePosition = true;
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    bool found = false;
    for (const auto& p : ann.parts) {
        if (p.kind == AnnouncementPartKind::Position &&
            p.text == "2 of 5") found = true;
    }
    EXPECT_TRUE(found);
}

TEST(AnnouncementEngine_Focus, PositionNotAnnouncedByDefault) {
    AccessNode n = MakeNode(AccessRole::ListItem, "Apple");
    n.positionInSet = 2;
    n.setSize = 5;
    AppContext ctx = MakeCtx();
    SpeechPolicy pol;   // announcePosition = false by default
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    for (const auto& p : ann.parts) {
        EXPECT_NE(p.kind, AnnouncementPartKind::Position);
    }
}

// ─── R7: Description ─────────────────────────────────────────────────────────

TEST(AnnouncementEngine_Focus, DescriptionAtDetailedVerbosity) {
    AccessNode n = MakeNode(AccessRole::Button, "OK", "", "Confirms the action");
    AppContext ctx = MakeCtx();
    SpeechPolicy pol = SpeechPolicy::Detailed();
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    bool found = false;
    for (const auto& p : ann.parts) {
        if (p.kind == AnnouncementPartKind::Description) found = true;
    }
    EXPECT_TRUE(found);
}

TEST(AnnouncementEngine_Focus, DescriptionNotAtStandardVerbosity) {
    AccessNode n = MakeNode(AccessRole::Button, "OK", "", "Confirms the action");
    AppContext ctx = MakeCtx();
    SpeechPolicy pol;   // Standard verbosity
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    for (const auto& p : ann.parts) {
        EXPECT_NE(p.kind, AnnouncementPartKind::Description);
    }
}

// ─── R9: Dialog context prefix ───────────────────────────────────────────────

TEST(AnnouncementEngine_Focus, DialogTitlePrependedOnModalEntry) {
    AccessNode n = MakeNode(AccessRole::Button, "OK");
    AppContext ctx = MakeCtx(AppContextType::Dialog, true, "Save As");
    SpeechPolicy pol;
    // prevTitle is different → new dialog → context part prepended
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol, "Main Window");
    ASSERT_FALSE(ann.parts.empty());
    EXPECT_EQ(ann.parts[0].kind, AnnouncementPartKind::Context);
    EXPECT_EQ(ann.parts[0].text, "Save As");
}

TEST(AnnouncementEngine_Focus, DialogTitleNotRepeatedForSameDialog) {
    AccessNode n = MakeNode(AccessRole::Button, "Cancel");
    AppContext ctx = MakeCtx(AppContextType::Dialog, true, "Save As");
    SpeechPolicy pol;
    // prevTitle matches current → no context part
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol, "Save As");
    for (const auto& p : ann.parts) {
        EXPECT_NE(p.kind, AnnouncementPartKind::Context);
    }
}

TEST(AnnouncementEngine_Focus, NonModalContextNoPrefixEvenWithTitle) {
    AccessNode n = MakeNode(AccessRole::Button, "Close");
    AppContext ctx = MakeCtx(AppContextType::DesktopApp, false, "My App");
    SpeechPolicy pol;
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol, "");
    for (const auto& p : ann.parts) {
        EXPECT_NE(p.kind, AnnouncementPartKind::Context);
    }
}

// ─── Flat string output ───────────────────────────────────────────────────────

TEST(AnnouncementEngine_Focus, FlattenButtonAnnouncement) {
    AccessNode n = MakeNode(AccessRole::Button, "Submit");
    AppContext ctx = MakeCtx();
    SpeechPolicy pol;
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    EXPECT_EQ(ann.Flatten(), "Submit, button");
}

TEST(AnnouncementEngine_Focus, FlattenCheckedCheckbox) {
    AccessNode n = MakeNode(AccessRole::CheckBox, "Bold", "", "",
                             AccessState::Checked);
    AppContext ctx = MakeCtx();
    SpeechPolicy pol;
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    const std::string flat = ann.Flatten();
    EXPECT_NE(flat.find("Bold"), std::string::npos);
    EXPECT_NE(flat.find("checkbox"), std::string::npos);
    EXPECT_NE(flat.find("checked"), std::string::npos);
}

TEST(AnnouncementEngine_Focus, FlattenPasswordEditNoValue) {
    AccessNode n = MakeNode(AccessRole::PasswordEdit, "Password", "hunter2");
    n.state = AccessState::Protected;
    AppContext ctx = MakeCtx();
    SpeechPolicy pol;
    Announcement ann = AnnouncementEngine::BuildFocusAnnouncement(n, ctx, pol);
    EXPECT_EQ(ann.Flatten().find("hunter2"), std::string::npos);
}

// ─── BuildPropertyAnnouncement ───────────────────────────────────────────────

TEST(AnnouncementEngine_Property, InvalidNodeIsEmpty) {
    AccessNode n = MakeNode(AccessRole::Slider, "Volume", "50%");
    n.isValid = false;
    AppContext ctx = MakeCtx();
    SpeechPolicy pol;
    Announcement ann = AnnouncementEngine::BuildPropertyAnnouncement(n, ctx, pol);
    EXPECT_TRUE(ann.IsEmpty());
}

TEST(AnnouncementEngine_Property, SliderValueAnnounced) {
    AccessNode n = MakeNode(AccessRole::Slider, "Volume", "80%");
    AppContext ctx = MakeCtx();
    SpeechPolicy pol;
    Announcement ann = AnnouncementEngine::BuildPropertyAnnouncement(n, ctx, pol);
    bool found = false;
    for (const auto& p : ann.parts) {
        if (p.kind == AnnouncementPartKind::Value &&
            p.text == "80%") found = true;
    }
    EXPECT_TRUE(found);
}

TEST(AnnouncementEngine_Property, DoesNotCancelPreviousSpeech) {
    AccessNode n = MakeNode(AccessRole::Slider, "Volume", "50%");
    AppContext ctx = MakeCtx();
    SpeechPolicy pol;
    Announcement ann = AnnouncementEngine::BuildPropertyAnnouncement(n, ctx, pol);
    EXPECT_FALSE(ann.cancelPrevious);
}

// ─── BuildAlertAnnouncement ───────────────────────────────────────────────────

TEST(AnnouncementEngine_Alert, PriorityIsHigh) {
    AccessNode n = MakeNode(AccessRole::Alert, "Disk full");
    AppContext ctx = MakeCtx();
    Announcement ann = AnnouncementEngine::BuildAlertAnnouncement(n, ctx);
    EXPECT_EQ(ann.priority, SpeechPriority::High);
}

TEST(AnnouncementEngine_Alert, CancelsPreviousSpeech) {
    AccessNode n = MakeNode(AccessRole::Alert, "Disk full");
    AppContext ctx = MakeCtx();
    Announcement ann = AnnouncementEngine::BuildAlertAnnouncement(n, ctx);
    EXPECT_TRUE(ann.cancelPrevious);
}

TEST(AnnouncementEngine_Alert, AlertRolePrependedToName) {
    AccessNode n = MakeNode(AccessRole::Alert, "Disk full");
    AppContext ctx = MakeCtx();
    Announcement ann = AnnouncementEngine::BuildAlertAnnouncement(n, ctx);
    const std::string flat = ann.Flatten();
    EXPECT_NE(flat.find("alert"), std::string::npos);
    EXPECT_NE(flat.find("Disk full"), std::string::npos);
}

TEST(AnnouncementEngine_Alert, AlertDialogRoleIsAnnounced) {
    AccessNode n = MakeNode(AccessRole::AlertDialog, "Confirm Delete");
    AppContext ctx = MakeCtx();
    Announcement ann = AnnouncementEngine::BuildAlertAnnouncement(n, ctx);
    const std::string flat = ann.Flatten();
    EXPECT_NE(flat.find("alert dialog"), std::string::npos);
}

TEST(AnnouncementEngine_Alert, InvalidNodeProducesEmptyAnnouncement) {
    AccessNode n = MakeNode(AccessRole::Alert, "Error");
    n.isValid = false;
    AppContext ctx = MakeCtx();
    Announcement ann = AnnouncementEngine::BuildAlertAnnouncement(n, ctx);
    EXPECT_TRUE(ann.IsEmpty());
}
