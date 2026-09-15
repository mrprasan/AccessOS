// AccessOS/src/Core/Context/ContextDetector.cpp

#include "ContextDetector.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace AccessOS {

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string ToLower(std::string_view sv) {
    std::string s(sv);
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return s;
}

static bool Contains(const std::string& haystack,
                     const char* needle) noexcept {
    return haystack.find(needle) != std::string::npos;
}

// ── Process name → AppContextType ─────────────────────────────────────────────

static constexpr struct { const char* proc; AppContextType type; const char* display; }
kProcessTable[] = {
    // Browsers
    { "chrome.exe",          AppContextType::Browser,     "Google Chrome"         },
    { "msedge.exe",          AppContextType::Browser,     "Microsoft Edge"        },
    { "firefox.exe",         AppContextType::Browser,     "Mozilla Firefox"       },
    { "brave.exe",           AppContextType::Browser,     "Brave"                 },
    { "opera.exe",           AppContextType::Browser,     "Opera"                 },
    { "iexplore.exe",        AppContextType::Browser,     "Internet Explorer"     },
    { "safari.exe",          AppContextType::Browser,     "Safari"                },
    { "vivaldi.exe",         AppContextType::Browser,     "Vivaldi"               },
    // Code editors / IDEs
    { "code.exe",            AppContextType::CodeEditor,  "Visual Studio Code"    },
    { "devenv.exe",          AppContextType::CodeEditor,  "Visual Studio"         },
    { "clion64.exe",         AppContextType::CodeEditor,  "CLion"                 },
    { "idea64.exe",          AppContextType::CodeEditor,  "IntelliJ IDEA"         },
    { "pycharm64.exe",       AppContextType::CodeEditor,  "PyCharm"               },
    { "webstorm64.exe",      AppContextType::CodeEditor,  "WebStorm"              },
    { "rider64.exe",         AppContextType::CodeEditor,  "Rider"                 },
    { "eclipse.exe",         AppContextType::CodeEditor,  "Eclipse"               },
    { "notepad++.exe",       AppContextType::CodeEditor,  "Notepad++"             },
    { "notepad.exe",         AppContextType::Document,    "Notepad"               },
    { "wordpad.exe",         AppContextType::Document,    "WordPad"               },
    // Terminals
    { "windowsterminal.exe", AppContextType::Terminal,    "Windows Terminal"      },
    { "cmd.exe",             AppContextType::Terminal,    "Command Prompt"        },
    { "powershell.exe",      AppContextType::Terminal,    "PowerShell"            },
    { "pwsh.exe",            AppContextType::Terminal,    "PowerShell 7"          },
    { "wsl.exe",             AppContextType::Terminal,    "WSL"                   },
    { "wt.exe",              AppContextType::Terminal,    "Windows Terminal"      },
    { "conhost.exe",         AppContextType::Terminal,    "Console Host"          },
    // Office / documents
    { "winword.exe",         AppContextType::Document,    "Microsoft Word"        },
    { "excel.exe",           AppContextType::Spreadsheet, "Microsoft Excel"       },
    { "powerpnt.exe",        AppContextType::Presentation,"Microsoft PowerPoint"  },
    { "swriter.exe",         AppContextType::Document,    "LibreOffice Writer"    },
    { "scalc.exe",           AppContextType::Spreadsheet, "LibreOffice Calc"      },
    { "simpress.exe",        AppContextType::Presentation,"LibreOffice Impress"   },
    { "acrord32.exe",        AppContextType::Document,    "Adobe Acrobat Reader"  },
    { "acrobat.exe",         AppContextType::Document,    "Adobe Acrobat"         },
    { "sumatra pdf.exe",     AppContextType::Document,    "Sumatra PDF"           },
    // Email / calendar
    { "outlook.exe",         AppContextType::Email,       "Microsoft Outlook"     },
    { "thunderbird.exe",     AppContextType::Email,       "Thunderbird"           },
    { "calendar.exe",        AppContextType::Calendar,    "Windows Calendar"      },
    // Chat / messaging
    { "teams.exe",           AppContextType::Chat,        "Microsoft Teams"       },
    { "slack.exe",           AppContextType::Chat,        "Slack"                 },
    { "discord.exe",         AppContextType::Chat,        "Discord"               },
    { "whatsapp.exe",        AppContextType::Chat,        "WhatsApp"              },
    { "telegram.exe",        AppContextType::Chat,        "Telegram"              },
    // Media
    { "vlc.exe",             AppContextType::MediaPlayer, "VLC Media Player"      },
    { "wmplayer.exe",        AppContextType::MediaPlayer, "Windows Media Player"  },
    { "spotify.exe",         AppContextType::MediaPlayer, "Spotify"               },
    // System
    { "explorer.exe",        AppContextType::FileExplorer,"File Explorer"         },
};

AppContextType ContextDetector::ClassifyByProcess(
    std::string_view procName) noexcept
{
    const std::string lower = ToLower(procName);
    for (const auto& e : kProcessTable) {
        if (lower == e.proc) return e.type;
    }
    return AppContextType::DesktopApp;
}

std::string ContextDetector::DisplayName(std::string_view procName) noexcept {
    const std::string lower = ToLower(procName);
    for (const auto& e : kProcessTable) {
        if (lower == e.proc) return e.display;
    }
    // Fallback: strip ".exe" and title-case.
    std::string name(procName);
    if (name.size() > 4) {
        const std::string ext = ToLower(name.substr(name.size() - 4));
        if (ext == ".exe") name = name.substr(0, name.size() - 4);
    }
    if (!name.empty()) name[0] = static_cast<char>(std::toupper(
        static_cast<unsigned char>(name[0])));
    return name;
}

// ── Window class → AppContextType ─────────────────────────────────────────────

AppContextType ContextDetector::ClassifyByWindowClass(
    std::string_view cls) noexcept
{
    const std::string lower = ToLower(cls);

    if (Contains(lower, "consolewindowclass")) return AppContextType::Terminal;
    if (Contains(lower, "virtualconsole"))     return AppContextType::Terminal;
    if (Contains(lower, "chrome_widgetwin"))   return AppContextType::Browser;
    if (Contains(lower, "mozillawindowclass")) return AppContextType::Browser;
    if (Contains(lower, "#32770"))             return AppContextType::Dialog;
    if (Contains(lower, "opusapp"))            return AppContextType::Chat;      // Teams
    if (Contains(lower, "cabviewwnd"))         return AppContextType::FileExplorer;
    if (Contains(lower, "shell_traywnd"))      return AppContextType::TaskBar;
    if (Contains(lower, "start"))              return AppContextType::StartMenu;

    return AppContextType::Unknown;
}

// ── Browser address bar refinement ────────────────────────────────────────────

AppContextType ContextDetector::RefineBrowserContext(
    const AccessNode& node) noexcept
{
    // Chrome / Edge address bar has a specific accessible name.
    const std::string nameLower = ToLower(node.name);
    if (Contains(nameLower, "address") ||
        Contains(nameLower, "location") ||
        Contains(nameLower, "url") ||
        Contains(nameLower, "search or enter address")) {
        return AppContextType::BrowserAddressBar;
    }
    return AppContextType::Browser;
}

// ── Main Detect ───────────────────────────────────────────────────────────────

AppContext ContextDetector::Detect(const AccessNode& node) noexcept {
    AppContext ctx;
    ctx.processId  = node.processId;
    ctx.windowTitle = node.windowTitle;

    // Derive process name from applicationName field.
    const std::string& appName = node.applicationName;
    ctx.processName = ToLower(appName);
    ctx.displayName = DisplayName(appName);

    // Classify by process name first (most reliable).
    ctx.type = ClassifyByProcess(appName);

    // If process lookup gave Unknown, try window class.
    if (ctx.type == AppContextType::Unknown ||
        ctx.type == AppContextType::DesktopApp) {
        const AppContextType byClass =
            ClassifyByWindowClass(node.windowTitle);
        if (byClass != AppContextType::Unknown) {
            ctx.type = byClass;
        }
    }

    // Detect modal dialog: role == Dialog or Dialog state.
    if (node.role == AccessRole::Dialog ||
        node.role == AccessRole::AlertDialog) {
        ctx.isModal = true;
        if (ctx.type == AppContextType::DesktopApp ||
            ctx.type == AppContextType::Unknown) {
            ctx.type = AppContextType::Dialog;
        }
    }

    // For browsers, refine to address bar if appropriate.
    if (ctx.IsBrowser() &&
        (node.role == AccessRole::Edit ||
         node.role == AccessRole::ComboBox)) {
        ctx.type = RefineBrowserContext(node);
    }

    // Mark web content for browser nodes that are not the address bar.
    if (ctx.type == AppContextType::Browser) {
        ctx.isWebContent = true;
    }

    return ctx;
}

} // namespace AccessOS
