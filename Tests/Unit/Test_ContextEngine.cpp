// AccessOS/Tests/Unit/Test_ContextEngine.cpp
//
// Unit tests for ContextEngine.
// Coverage: initial state, Update detects change, no-op on same context,
//           observer notification, multiple observers, ClearObservers,
//           ObserverCount, GetCurrent thread-safety (single-threaded proxy).

#include <gtest/gtest.h>
#include "Context/ContextEngine.h"
#include "Semantic/AccessNode.h"

using namespace AccessOS;

// ── Helpers ───────────────────────────────────────────────────────────────────

static AccessNode MakeNode(const std::string& appName,
                            AccessRole role = AccessRole::Window,
                            const std::string& name = "",
                            uint32_t pid = 1)
{
    AccessNode n;
    n.applicationName = appName;
    n.windowTitle     = appName + " window";
    n.role            = role;
    n.name            = name;
    n.processId       = pid;
    return n;
}

// ─── Initial state ────────────────────────────────────────────────────────────

TEST(ContextEngine, InitialContextIsUnknown) {
    ContextEngine engine;
    AppContext ctx = engine.GetCurrent();
    EXPECT_EQ(ctx.type, AppContextType::Unknown);
    EXPECT_EQ(ctx.processId, 0u);
    EXPECT_EQ(ctx.processName, "");
}

TEST(ContextEngine, InitialObserverCountIsZero) {
    ContextEngine engine;
    EXPECT_EQ(engine.ObserverCount(), 0u);
}

// ─── Update ───────────────────────────────────────────────────────────────────

TEST(ContextEngine, UpdateFromUnknownToChrome) {
    ContextEngine engine;
    bool changed = engine.Update(MakeNode("chrome.exe", AccessRole::Window, "", 100));
    EXPECT_TRUE(changed);
    AppContext ctx = engine.GetCurrent();
    EXPECT_EQ(ctx.type, AppContextType::Browser);
    EXPECT_EQ(ctx.processId, 100u);
    EXPECT_TRUE(ctx.isWebContent);
}

TEST(ContextEngine, UpdateWithSameProcessNoChange) {
    ContextEngine engine;
    AccessNode n = MakeNode("chrome.exe", AccessRole::Window, "", 100);
    engine.Update(n);                  // first update — changes
    bool changed = engine.Update(n);   // same node again — no change
    EXPECT_FALSE(changed);
}

TEST(ContextEngine, UpdateToNewProcessReturnsTrue) {
    ContextEngine engine;
    engine.Update(MakeNode("chrome.exe", AccessRole::Window, "", 100));
    bool changed = engine.Update(MakeNode("code.exe", AccessRole::Window, "", 200));
    EXPECT_TRUE(changed);
    AppContext ctx = engine.GetCurrent();
    EXPECT_EQ(ctx.type, AppContextType::CodeEditor);
    EXPECT_EQ(ctx.processId, 200u);
}

TEST(ContextEngine, GetCurrentReflectsLastUpdate) {
    ContextEngine engine;
    engine.Update(MakeNode("teams.exe", AccessRole::Window, "", 42));
    AppContext ctx = engine.GetCurrent();
    EXPECT_EQ(ctx.type, AppContextType::Chat);
    EXPECT_EQ(ctx.processId, 42u);
}

TEST(ContextEngine, UpdateDialogSetsModalFlag) {
    ContextEngine engine;
    engine.Update(MakeNode("notepad.exe", AccessRole::Window, "", 10));
    engine.Update(MakeNode("notepad.exe", AccessRole::AlertDialog, "", 10));
    AppContext ctx = engine.GetCurrent();
    EXPECT_TRUE(ctx.isModal);
}

// ─── Observers ────────────────────────────────────────────────────────────────

TEST(ContextEngine, ObserverCalledOnChange) {
    ContextEngine engine;
    int callCount = 0;
    AppContext captured;
    engine.AddObserver([&](const AppContext& c) {
        ++callCount;
        captured = c;
    });
    engine.Update(MakeNode("chrome.exe", AccessRole::Window, "", 5));
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(captured.type, AppContextType::Browser);
}

TEST(ContextEngine, ObserverNotCalledWhenContextUnchanged) {
    ContextEngine engine;
    int callCount = 0;
    engine.AddObserver([&](const AppContext&) { ++callCount; });
    AccessNode n = MakeNode("chrome.exe", AccessRole::Window, "", 5);
    engine.Update(n);
    engine.Update(n);
    EXPECT_EQ(callCount, 1);   // fired once on first change only
}

TEST(ContextEngine, MultipleObserversAllFired) {
    ContextEngine engine;
    int a = 0, b = 0, c = 0;
    engine.AddObserver([&](const AppContext&) { ++a; });
    engine.AddObserver([&](const AppContext&) { ++b; });
    engine.AddObserver([&](const AppContext&) { ++c; });
    engine.Update(MakeNode("code.exe", AccessRole::Window, "", 7));
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 1);
    EXPECT_EQ(c, 1);
}

TEST(ContextEngine, ObserverCountMatchesAdded) {
    ContextEngine engine;
    EXPECT_EQ(engine.ObserverCount(), 0u);
    engine.AddObserver([](const AppContext&) {});
    EXPECT_EQ(engine.ObserverCount(), 1u);
    engine.AddObserver([](const AppContext&) {});
    EXPECT_EQ(engine.ObserverCount(), 2u);
}

TEST(ContextEngine, ClearObserversRemovesAll) {
    ContextEngine engine;
    engine.AddObserver([](const AppContext&) {});
    engine.AddObserver([](const AppContext&) {});
    engine.ClearObservers();
    EXPECT_EQ(engine.ObserverCount(), 0u);
}

TEST(ContextEngine, ObserverNotCalledAfterClear) {
    ContextEngine engine;
    int callCount = 0;
    engine.AddObserver([&](const AppContext&) { ++callCount; });
    engine.ClearObservers();
    engine.Update(MakeNode("chrome.exe", AccessRole::Window, "", 1));
    EXPECT_EQ(callCount, 0);
}

TEST(ContextEngine, ObserverReceivesCorrectContextType) {
    ContextEngine engine;
    AppContextType received = AppContextType::Unknown;
    engine.AddObserver([&](const AppContext& c) { received = c.type; });
    engine.Update(MakeNode("excel.exe", AccessRole::Window, "", 8));
    EXPECT_EQ(received, AppContextType::Spreadsheet);
}

// ─── Context transitions ──────────────────────────────────────────────────────

TEST(ContextEngine, TransitionBrowserToTerminal) {
    ContextEngine engine;
    engine.Update(MakeNode("chrome.exe", AccessRole::Window, "", 1));
    engine.Update(MakeNode("cmd.exe",    AccessRole::Window, "", 2));
    AppContext ctx = engine.GetCurrent();
    EXPECT_EQ(ctx.type, AppContextType::Terminal);
    EXPECT_TRUE(ctx.IsDeveloperTool());
}

TEST(ContextEngine, TransitionClearsWebContentForTerminal) {
    ContextEngine engine;
    engine.Update(MakeNode("chrome.exe", AccessRole::Window, "", 1));
    EXPECT_TRUE(engine.GetCurrent().isWebContent);
    engine.Update(MakeNode("cmd.exe", AccessRole::Window, "", 2));
    EXPECT_FALSE(engine.GetCurrent().isWebContent);
}

TEST(ContextEngine, SamePidDifferentRoleIsChange) {
    ContextEngine engine;
    // First: normal window; second: dialog (isModal flips)
    engine.Update(MakeNode("notepad.exe", AccessRole::Window, "", 10));
    EXPECT_FALSE(engine.GetCurrent().isModal);
    bool changed = engine.Update(MakeNode("notepad.exe", AccessRole::AlertDialog, "", 10));
    EXPECT_TRUE(changed);
    EXPECT_TRUE(engine.GetCurrent().isModal);
}

TEST(ContextEngine, UpdateUnknownProcessGivesDesktopApp) {
    ContextEngine engine;
    engine.Update(MakeNode("mypaint.exe", AccessRole::Window, "", 77));
    AppContext ctx = engine.GetCurrent();
    EXPECT_EQ(ctx.type, AppContextType::DesktopApp);
    EXPECT_FALSE(ctx.isWebContent);
    EXPECT_FALSE(ctx.isModal);
}
