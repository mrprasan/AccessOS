// AccessOS/Tests/Unit/Test_ContextDetector.cpp
//
// Unit tests for ContextDetector.
// Coverage: ClassifyByProcess, ClassifyByWindowClass, DisplayName,
//           RefineBrowserContext, Detect end-to-end.

#include <gtest/gtest.h>
#include "Context/ContextDetector.h"
#include "Semantic/AccessNode.h"

using namespace AccessOS;

// ── Helpers ───────────────────────────────────────────────────────────────────

static AccessNode MakeNode(const std::string& appName,
                            const std::string& windowTitle = "",
                            AccessRole role = AccessRole::Window,
                            const std::string& name = "",
                            uint32_t pid = 1234)
{
    AccessNode n;
    n.applicationName = appName;
    n.windowTitle     = windowTitle;
    n.role            = role;
    n.name            = name;
    n.processId       = pid;
    return n;
}

// ─── ClassifyByProcess ────────────────────────────────────────────────────────

TEST(ContextDetector_ClassifyByProcess, KnownBrowsers) {
    EXPECT_EQ(ContextDetector::ClassifyByProcess("chrome.exe"),  AppContextType::Browser);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("msedge.exe"),  AppContextType::Browser);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("firefox.exe"), AppContextType::Browser);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("brave.exe"),   AppContextType::Browser);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("opera.exe"),   AppContextType::Browser);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("vivaldi.exe"), AppContextType::Browser);
}

TEST(ContextDetector_ClassifyByProcess, BrowsersCaseInsensitive) {
    EXPECT_EQ(ContextDetector::ClassifyByProcess("CHROME.EXE"),  AppContextType::Browser);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("Chrome.Exe"),  AppContextType::Browser);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("MSEDGE.EXE"),  AppContextType::Browser);
}

TEST(ContextDetector_ClassifyByProcess, CodeEditors) {
    EXPECT_EQ(ContextDetector::ClassifyByProcess("code.exe"),       AppContextType::CodeEditor);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("devenv.exe"),     AppContextType::CodeEditor);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("notepad++.exe"),  AppContextType::CodeEditor);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("clion64.exe"),    AppContextType::CodeEditor);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("idea64.exe"),     AppContextType::CodeEditor);
}

TEST(ContextDetector_ClassifyByProcess, Terminals) {
    EXPECT_EQ(ContextDetector::ClassifyByProcess("windowsterminal.exe"), AppContextType::Terminal);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("cmd.exe"),             AppContextType::Terminal);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("powershell.exe"),      AppContextType::Terminal);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("pwsh.exe"),            AppContextType::Terminal);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("wsl.exe"),             AppContextType::Terminal);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("conhost.exe"),         AppContextType::Terminal);
}

TEST(ContextDetector_ClassifyByProcess, Documents) {
    EXPECT_EQ(ContextDetector::ClassifyByProcess("winword.exe"),   AppContextType::Document);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("notepad.exe"),   AppContextType::Document);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("wordpad.exe"),   AppContextType::Document);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("acrord32.exe"),  AppContextType::Document);
}

TEST(ContextDetector_ClassifyByProcess, Spreadsheets) {
    EXPECT_EQ(ContextDetector::ClassifyByProcess("excel.exe"),  AppContextType::Spreadsheet);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("scalc.exe"),  AppContextType::Spreadsheet);
}

TEST(ContextDetector_ClassifyByProcess, Presentations) {
    EXPECT_EQ(ContextDetector::ClassifyByProcess("powerpnt.exe"), AppContextType::Presentation);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("simpress.exe"), AppContextType::Presentation);
}

TEST(ContextDetector_ClassifyByProcess, EmailAndChat) {
    EXPECT_EQ(ContextDetector::ClassifyByProcess("outlook.exe"),     AppContextType::Email);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("thunderbird.exe"), AppContextType::Email);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("teams.exe"),       AppContextType::Chat);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("slack.exe"),       AppContextType::Chat);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("discord.exe"),     AppContextType::Chat);
}

TEST(ContextDetector_ClassifyByProcess, Media) {
    EXPECT_EQ(ContextDetector::ClassifyByProcess("vlc.exe"),       AppContextType::MediaPlayer);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("spotify.exe"),   AppContextType::MediaPlayer);
    EXPECT_EQ(ContextDetector::ClassifyByProcess("wmplayer.exe"),  AppContextType::MediaPlayer);
}

TEST(ContextDetector_ClassifyByProcess, FileExplorer) {
    EXPECT_EQ(ContextDetector::ClassifyByProcess("explorer.exe"), AppContextType::FileExplorer);
}

TEST(ContextDetector_ClassifyByProcess, UnknownProcess) {
    EXPECT_EQ(ContextDetector::ClassifyByProcess("mypaint.exe"), AppContextType::DesktopApp);
    EXPECT_EQ(ContextDetector::ClassifyByProcess(""),            AppContextType::DesktopApp);
}

// ─── DisplayName ─────────────────────────────────────────────────────────────

TEST(ContextDetector_DisplayName, KnownApps) {
    EXPECT_EQ(ContextDetector::DisplayName("chrome.exe"),  "Google Chrome");
    EXPECT_EQ(ContextDetector::DisplayName("code.exe"),    "Visual Studio Code");
    EXPECT_EQ(ContextDetector::DisplayName("teams.exe"),   "Microsoft Teams");
    EXPECT_EQ(ContextDetector::DisplayName("vlc.exe"),     "VLC Media Player");
}

TEST(ContextDetector_DisplayName, UnknownStripsExeAndCapitalises) {
    const std::string dn = ContextDetector::DisplayName("myprog.exe");
    EXPECT_EQ(dn, "Myprog");
}

TEST(ContextDetector_DisplayName, UnknownNoExtension) {
    const std::string dn = ContextDetector::DisplayName("myprog");
    EXPECT_EQ(dn, "Myprog");
}

TEST(ContextDetector_DisplayName, EmptyProcess) {
    EXPECT_EQ(ContextDetector::DisplayName(""), "");
}

// ─── ClassifyByWindowClass ────────────────────────────────────────────────────

TEST(ContextDetector_ClassifyByWindowClass, ConsoleWindowClass) {
    EXPECT_EQ(ContextDetector::ClassifyByWindowClass("ConsoleWindowClass"),
              AppContextType::Terminal);
}

TEST(ContextDetector_ClassifyByWindowClass, ChromeWidget) {
    EXPECT_EQ(ContextDetector::ClassifyByWindowClass("Chrome_WidgetWin_1"),
              AppContextType::Browser);
}

TEST(ContextDetector_ClassifyByWindowClass, MozillaWindow) {
    EXPECT_EQ(ContextDetector::ClassifyByWindowClass("MozillaWindowClass"),
              AppContextType::Browser);
}

TEST(ContextDetector_ClassifyByWindowClass, DialogClass) {
    EXPECT_EQ(ContextDetector::ClassifyByWindowClass("#32770"),
              AppContextType::Dialog);
}

TEST(ContextDetector_ClassifyByWindowClass, TeamsOpusApp) {
    EXPECT_EQ(ContextDetector::ClassifyByWindowClass("OpusApp"),
              AppContextType::Chat);
}

TEST(ContextDetector_ClassifyByWindowClass, ShellTrayWindow) {
    EXPECT_EQ(ContextDetector::ClassifyByWindowClass("Shell_TrayWnd"),
              AppContextType::TaskBar);
}

TEST(ContextDetector_ClassifyByWindowClass, Unknown) {
    EXPECT_EQ(ContextDetector::ClassifyByWindowClass("SomeRandomClass"),
              AppContextType::Unknown);
}

// ─── RefineBrowserContext ─────────────────────────────────────────────────────

TEST(ContextDetector_RefineBrowserContext, AddressBarByName) {
    AccessNode n = MakeNode("chrome.exe", "", AccessRole::Edit, "Address and search bar");
    EXPECT_EQ(ContextDetector::RefineBrowserContext(n),
              AppContextType::BrowserAddressBar);
}

TEST(ContextDetector_RefineBrowserContext, LocationBarByName) {
    AccessNode n = MakeNode("firefox.exe", "", AccessRole::Edit, "Search or enter address");
    EXPECT_EQ(ContextDetector::RefineBrowserContext(n),
              AppContextType::BrowserAddressBar);
}

TEST(ContextDetector_RefineBrowserContext, UrlContained) {
    AccessNode n = MakeNode("msedge.exe", "", AccessRole::Edit, "Enter URL");
    EXPECT_EQ(ContextDetector::RefineBrowserContext(n),
              AppContextType::BrowserAddressBar);
}

TEST(ContextDetector_RefineBrowserContext, NormalEditIsNotAddressBar) {
    AccessNode n = MakeNode("chrome.exe", "", AccessRole::Edit, "Search Google");
    EXPECT_EQ(ContextDetector::RefineBrowserContext(n),
              AppContextType::Browser);
}

// ─── Detect end-to-end ───────────────────────────────────────────────────────

TEST(ContextDetector_Detect, ChromeWindowNode) {
    AccessNode n = MakeNode("chrome.exe", "Google Chrome", AccessRole::Window, "", 100);
    AppContext ctx = ContextDetector::Detect(n);
    EXPECT_EQ(ctx.type,        AppContextType::Browser);
    EXPECT_EQ(ctx.processId,   100u);
    EXPECT_TRUE(ctx.isWebContent);
    EXPECT_FALSE(ctx.isModal);
    EXPECT_EQ(ctx.displayName, "Google Chrome");
}

TEST(ContextDetector_Detect, ChromeAddressBarEdit) {
    AccessNode n = MakeNode("chrome.exe", "New Tab", AccessRole::Edit,
                            "Address and search bar", 200);
    AppContext ctx = ContextDetector::Detect(n);
    EXPECT_EQ(ctx.type, AppContextType::BrowserAddressBar);
    EXPECT_FALSE(ctx.isWebContent);   // address bar is not web content
}

TEST(ContextDetector_Detect, VSCodeNode) {
    AccessNode n = MakeNode("Code.exe", "main.cpp - Visual Studio Code",
                            AccessRole::Window, "", 300);
    AppContext ctx = ContextDetector::Detect(n);
    EXPECT_EQ(ctx.type, AppContextType::CodeEditor);
    EXPECT_EQ(ctx.displayName, "Visual Studio Code");
    EXPECT_FALSE(ctx.isWebContent);
}

TEST(ContextDetector_Detect, DialogNode) {
    AccessNode n = MakeNode("notepad.exe", "Save As", AccessRole::Dialog, "", 400);
    AppContext ctx = ContextDetector::Detect(n);
    EXPECT_TRUE(ctx.isModal);
    EXPECT_EQ(ctx.type, AppContextType::Document);  // process gave Document; modal flag set
}

TEST(ContextDetector_Detect, AlertDialog) {
    AccessNode n = MakeNode("unknown.exe", "Error", AccessRole::AlertDialog, "", 500);
    AppContext ctx = ContextDetector::Detect(n);
    EXPECT_TRUE(ctx.isModal);
    EXPECT_EQ(ctx.type, AppContextType::Dialog);
}

TEST(ContextDetector_Detect, UnknownProcessFallback) {
    AccessNode n = MakeNode("mygame.exe", "My Game", AccessRole::Window, "", 600);
    AppContext ctx = ContextDetector::Detect(n);
    EXPECT_EQ(ctx.type, AppContextType::DesktopApp);
    EXPECT_FALSE(ctx.isWebContent);
    EXPECT_FALSE(ctx.isModal);
}

TEST(ContextDetector_Detect, TerminalByProcess) {
    AccessNode n = MakeNode("WindowsTerminal.exe", "Windows Terminal",
                            AccessRole::Window, "", 700);
    AppContext ctx = ContextDetector::Detect(n);
    EXPECT_EQ(ctx.type, AppContextType::Terminal);
    EXPECT_TRUE(ctx.IsDeveloperTool());
}

TEST(ContextDetector_Detect, ProcessIdPropagated) {
    AccessNode n = MakeNode("excel.exe", "Book1.xlsx", AccessRole::Window, "", 999);
    AppContext ctx = ContextDetector::Detect(n);
    EXPECT_EQ(ctx.processId, 999u);
    EXPECT_EQ(ctx.type, AppContextType::Spreadsheet);
}
