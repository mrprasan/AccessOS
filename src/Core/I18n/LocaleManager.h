// LocaleManager.h — Localisation / i18n subsystem (ACCESSOS-043)
//
// Provides string lookup by key with fallback to English (en-US).
// Message catalogues are loaded from embedded resource maps defined at
// compile time. Runtime locale is derived from GetUserDefaultLocaleName()
// and can be overridden via SetLocale().
//
// Design:
//   - No external file I/O — all strings are compiled into the binary.
//   - String IDs are strongly-typed (enum class MsgId).
//   - Fallback chain: requested locale → "en" → raw key string.
//   - Thread-safe: all public methods are const or use atomic locale ID.

#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace AccessOS {
namespace I18n {

// ── Message IDs ────────────────────────────────────────────────────────────────
// Every user-facing string has an ID. Add new IDs here; add translations below.
enum class MsgId : uint32_t {
    // Focus announcements
    FocusedOn = 0,
    RoleButton,
    RoleLink,
    RoleEdit,
    RoleCheckBox,
    RoleRadioButton,
    RoleComboBox,
    RoleListItem,
    RoleListBox,
    RoleMenu,
    RoleMenuItem,
    RoleDialog,
    RoleWindow,
    RoleToolBar,
    RoleStatusBar,
    RoleTab,
    RoleTabItem,
    RoleTree,
    RoleTreeItem,
    RoleTable,
    RoleDocument,
    RoleHeading,

    // State words
    StateChecked,
    StateUnchecked,
    StateExpanded,
    StateCollapsed,
    StateSelected,
    StateDisabled,
    StateReadOnly,
    StateRequired,
    StateProtected,

    // Browse mode
    BrowseModeOn,
    BrowseModeOff,
    BrowseModeHeading,
    BrowseModeLink,
    BrowseModeFormField,
    BrowseModeTable,
    BrowseModeEndOfDocument,

    // Speech
    SpeechStarted,
    SpeechStopped,
    SpeechPaused,
    SpeechResumed,

    // Table navigation
    TableRow,
    TableColumn,
    TableCell,
    TableColumnHeader,
    TableRowHeader,

    // Clipboard
    ClipboardEmpty,
    ClipboardText,

    // System
    ScreenReaderStarted,
    ScreenReaderStopped,
    HelpTitle,

    _Count  // sentinel — keep last
};

// ── LocaleManager ──────────────────────────────────────────────────────────────

class LocaleManager {
public:
    // Singleton — one locale manager per process.
    static LocaleManager& Instance();

    // ── Locale selection ──────────────────────────────────────────────────────

    // Detect and set locale from Windows (GetUserDefaultLocaleName).
    // Returns the locale tag that was set (e.g. "en", "fr", "de").
    std::string AutoDetect();

    // Override locale (e.g. from settings). Tag examples: "en", "fr", "de".
    // Returns true if the locale has a registered catalogue.
    bool SetLocale(const std::string& tag);

    // Returns the current locale tag.
    const std::string& GetLocale() const;

    // Returns all locale tags that have registered catalogues.
    std::vector<std::string> AvailableLocales() const;

    // ── String lookup ─────────────────────────────────────────────────────────

    // Look up a message by ID. Falls back to "en" then to the enum name.
    const std::string& Get(MsgId id) const;

    // Look up with one substitution placeholder {0}.
    std::string Format(MsgId id, const std::string& arg0) const;

    // Look up with two substitution placeholders {0}, {1}.
    std::string Format(MsgId id,
                       const std::string& arg0,
                       const std::string& arg1) const;

private:
    LocaleManager();

    using Catalogue = std::unordered_map<uint32_t, std::string>;

    // Registered catalogues: locale tag → message map
    std::unordered_map<std::string, Catalogue> m_catalogues;
    std::string m_locale; // current locale tag (e.g. "en")

    // Register built-in catalogues.
    void RegisterBuiltins();
    void RegisterEn();
    void RegisterFr();
    void RegisterDe();

    const Catalogue* FindCatalogue(const std::string& tag) const;
    static std::string SubstituteArgs(const std::string& tmpl,
                                       const std::string& a0,
                                       const std::string& a1 = "");
};

// ── Convenience free function ──────────────────────────────────────────────────

inline const std::string& Msg(MsgId id) {
    return LocaleManager::Instance().Get(id);
}

inline std::string MsgFmt(MsgId id, const std::string& a0) {
    return LocaleManager::Instance().Format(id, a0);
}

} // namespace I18n
} // namespace AccessOS
