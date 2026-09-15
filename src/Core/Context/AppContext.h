// AccessOS/src/Core/Context/AppContext.h
//
// AppContext — describes the application environment surrounding the focused element.
//
// Why: Different applications require different speech behaviour.
//   - A browser's address bar should be read differently from a document edit field.
//   - A terminal prompt should not announce "edit" on every character.
//   - A dialog confirmation button needs less context than the same button in an IDE.
//
// Context is detected once per application switch, not per element focus.
// Speech and navigation components read it to tune their output.

#pragma once

#include <string>
#include <cstdint>
#include <functional>

namespace AccessOS {

/// High-level category of the active application.
enum class AppContextType : uint8_t {
    Unknown = 0,

    // Web browsers.
    Browser,            // Chrome, Edge, Firefox, Safari, Opera, Brave
    BrowserAddressBar,  // Address/URL bar — suppress role, announce URL

    // Document / office.
    Document,           // Word, LibreOffice Writer, Acrobat
    Spreadsheet,        // Excel, Calc
    Presentation,       // PowerPoint, Impress

    // Code / terminal.
    CodeEditor,         // VS Code, Visual Studio, CLion, IntelliJ
    Terminal,           // Windows Terminal, CMD, PowerShell, WSL

    // System UI.
    Dialog,             // Modal dialog — read title + focused element concisely
    FileExplorer,       // Windows Explorer
    SystemTray,         // System tray / notification area
    TaskBar,            // Windows taskbar
    StartMenu,          // Start menu

    // Messaging / productivity.
    Email,              // Outlook, Thunderbird
    Chat,               // Teams, Slack, Discord, WhatsApp
    Calendar,           // Outlook calendar, Windows Calendar

    // Media.
    MediaPlayer,        // VLC, Windows Media Player, Spotify

    // General desktop app.
    DesktopApp,         // Catch-all for unrecognised GUI apps
};

/// A snapshot of the current application context.
struct AppContext {
    AppContextType  type            = AppContextType::Unknown;

    /// Lowercase executable name, e.g. "chrome.exe", "code.exe".
    std::string     processName;

    /// Window title as reported by UIA/Win32.
    std::string     windowTitle;

    /// Win32 window class name.
    std::string     windowClassName;

    uint32_t        processId       = 0;

    /// True when the focused element is inside a web browser's content area.
    bool            isWebContent    = false;

    /// True when the active window is a modal dialog.
    bool            isModal         = false;

    /// Human-readable label, e.g. "Google Chrome", "Visual Studio Code".
    std::string     displayName;

    /// Returns true if this is any browser context.
    bool IsBrowser() const noexcept {
        return type == AppContextType::Browser ||
               type == AppContextType::BrowserAddressBar;
    }

    /// Returns true if this is a code or terminal context.
    bool IsDeveloperTool() const noexcept {
        return type == AppContextType::CodeEditor ||
               type == AppContextType::Terminal;
    }
};

/// Callback signature for context-change observers.
/// Invoked on the EventEngine worker thread — must not block.
using ContextObserver = std::function<void(const AppContext&)>;

} // namespace AccessOS
