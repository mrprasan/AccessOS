// LocaleManager.cpp — Localisation / i18n subsystem (ACCESSOS-043)

#include "LocaleManager.h"
#include <Windows.h>
#include <algorithm>

namespace AccessOS {
namespace I18n {

// ── Singleton ─────────────────────────────────────────────────────────────────

LocaleManager& LocaleManager::Instance() {
    static LocaleManager inst;
    return inst;
}

LocaleManager::LocaleManager() {
    RegisterBuiltins();
    AutoDetect();
}

// ── Locale selection ──────────────────────────────────────────────────────────

std::string LocaleManager::AutoDetect() {
    wchar_t buf[LOCALE_NAME_MAX_LENGTH] = {};
    if (::GetUserDefaultLocaleName(buf, LOCALE_NAME_MAX_LENGTH) > 0) {
        // Convert "en-US" → "en", "fr-FR" → "fr", etc.
        int len = ::WideCharToMultiByte(CP_UTF8, 0, buf, -1,
                                         nullptr, 0, nullptr, nullptr);
        std::string tag;
        if (len > 0) {
            tag.resize(static_cast<size_t>(len) - 1);
            ::WideCharToMultiByte(CP_UTF8, 0, buf, -1,
                                   tag.data(), len, nullptr, nullptr);
        }
        // Strip the region suffix: "en-US" → "en"
        auto dash = tag.find('-');
        if (dash != std::string::npos) tag = tag.substr(0, dash);

        if (SetLocale(tag)) return tag;
    }
    // Fallback to English
    SetLocale("en");
    return "en";
}

bool LocaleManager::SetLocale(const std::string& tag) {
    if (m_catalogues.count(tag)) {
        m_locale = tag;
        return true;
    }
    return false;
}

const std::string& LocaleManager::GetLocale() const {
    return m_locale;
}

std::vector<std::string> LocaleManager::AvailableLocales() const {
    std::vector<std::string> out;
    out.reserve(m_catalogues.size());
    for (auto& [k, _] : m_catalogues) out.push_back(k);
    return out;
}

// ── String lookup ─────────────────────────────────────────────────────────────

const std::string& LocaleManager::Get(MsgId id) const {
    static const std::string kEmpty;
    uint32_t key = static_cast<uint32_t>(id);

    // Try current locale
    auto* cat = FindCatalogue(m_locale);
    if (cat) {
        auto it = cat->find(key);
        if (it != cat->end()) return it->second;
    }
    // Fallback to English
    if (m_locale != "en") {
        auto* en = FindCatalogue("en");
        if (en) {
            auto it = en->find(key);
            if (it != en->end()) return it->second;
        }
    }
    return kEmpty;
}

std::string LocaleManager::Format(MsgId id, const std::string& arg0) const {
    return SubstituteArgs(Get(id), arg0);
}

std::string LocaleManager::Format(MsgId id,
                                   const std::string& arg0,
                                   const std::string& arg1) const {
    return SubstituteArgs(Get(id), arg0, arg1);
}

// ── private helpers ───────────────────────────────────────────────────────────

const LocaleManager::Catalogue* LocaleManager::FindCatalogue(
        const std::string& tag) const {
    auto it = m_catalogues.find(tag);
    if (it == m_catalogues.end()) return nullptr;
    return &it->second;
}

std::string LocaleManager::SubstituteArgs(const std::string& tmpl,
                                           const std::string& a0,
                                           const std::string& a1) {
    std::string out = tmpl;
    // Replace {0} with a0
    size_t pos = out.find("{0}");
    if (pos != std::string::npos) out.replace(pos, 3, a0);
    // Replace {1} with a1
    pos = out.find("{1}");
    if (pos != std::string::npos) out.replace(pos, 3, a1);
    return out;
}

// ── Built-in catalogue registration ──────────────────────────────────────────

void LocaleManager::RegisterBuiltins() {
    RegisterEn();
    RegisterFr();
    RegisterDe();
}

#define MSG(id, str) { static_cast<uint32_t>(MsgId::id), str }

void LocaleManager::RegisterEn() {
    m_catalogues["en"] = {
        MSG(FocusedOn,           "{0}"),
        MSG(RoleButton,          "button"),
        MSG(RoleLink,            "link"),
        MSG(RoleEdit,            "edit"),
        MSG(RoleCheckBox,        "check box"),
        MSG(RoleRadioButton,     "radio button"),
        MSG(RoleComboBox,        "combo box"),
        MSG(RoleListItem,        "list item"),
        MSG(RoleListBox,         "list"),
        MSG(RoleMenu,            "menu"),
        MSG(RoleMenuItem,        "menu item"),
        MSG(RoleDialog,          "dialog"),
        MSG(RoleWindow,          "window"),
        MSG(RoleToolBar,         "toolbar"),
        MSG(RoleStatusBar,       "status bar"),
        MSG(RoleTab,             "tab"),
        MSG(RoleTabItem,         "tab item"),
        MSG(RoleTree,            "tree"),
        MSG(RoleTreeItem,        "tree item"),
        MSG(RoleTable,           "table"),
        MSG(RoleDocument,        "document"),
        MSG(RoleHeading,         "heading"),
        MSG(StateChecked,        "checked"),
        MSG(StateUnchecked,      "unchecked"),
        MSG(StateExpanded,       "expanded"),
        MSG(StateCollapsed,      "collapsed"),
        MSG(StateSelected,       "selected"),
        MSG(StateDisabled,       "disabled"),
        MSG(StateReadOnly,       "read only"),
        MSG(StateRequired,       "required"),
        MSG(StateProtected,      "protected"),
        MSG(BrowseModeOn,        "Browse mode on"),
        MSG(BrowseModeOff,       "Browse mode off"),
        MSG(BrowseModeHeading,   "heading"),
        MSG(BrowseModeLink,      "link"),
        MSG(BrowseModeFormField, "form field"),
        MSG(BrowseModeTable,     "table"),
        MSG(BrowseModeEndOfDocument, "end of document"),
        MSG(SpeechStarted,       "screen reader started"),
        MSG(SpeechStopped,       "screen reader stopped"),
        MSG(SpeechPaused,        "speech paused"),
        MSG(SpeechResumed,       "speech resumed"),
        MSG(TableRow,            "row {0}"),
        MSG(TableColumn,         "column {0}"),
        MSG(TableCell,           "{0}, row {1}"),
        MSG(TableColumnHeader,   "column header {0}"),
        MSG(TableRowHeader,      "row header {0}"),
        MSG(ClipboardEmpty,      "clipboard is empty"),
        MSG(ClipboardText,       "{0}"),
        MSG(ScreenReaderStarted, "AccessOS screen reader started"),
        MSG(ScreenReaderStopped, "AccessOS screen reader stopped"),
        MSG(HelpTitle,           "AccessOS keyboard shortcuts"),
    };
}

void LocaleManager::RegisterFr() {
    m_catalogues["fr"] = {
        MSG(RoleButton,          "bouton"),
        MSG(RoleLink,            "lien"),
        MSG(RoleEdit,            "champ de texte"),
        MSG(RoleCheckBox,        "case à cocher"),
        MSG(RoleRadioButton,     "bouton radio"),
        MSG(RoleComboBox,        "liste déroulante"),
        MSG(RoleListItem,        "élément de liste"),
        MSG(RoleMenu,            "menu"),
        MSG(RoleMenuItem,        "élément de menu"),
        MSG(RoleDialog,          "boîte de dialogue"),
        MSG(RoleWindow,          "fenêtre"),
        MSG(StateChecked,        "coché"),
        MSG(StateUnchecked,      "non coché"),
        MSG(StateDisabled,       "désactivé"),
        MSG(StateSelected,       "sélectionné"),
        MSG(BrowseModeOn,        "mode navigation activé"),
        MSG(BrowseModeOff,       "mode navigation désactivé"),
        MSG(BrowseModeEndOfDocument, "fin du document"),
        MSG(ClipboardEmpty,      "le presse-papiers est vide"),
        MSG(ScreenReaderStarted, "lecteur d'écran AccessOS démarré"),
        MSG(ScreenReaderStopped, "lecteur d'écran AccessOS arrêté"),
    };
}

void LocaleManager::RegisterDe() {
    m_catalogues["de"] = {
        MSG(RoleButton,          "Schaltfläche"),
        MSG(RoleLink,            "Link"),
        MSG(RoleEdit,            "Bearbeitungsfeld"),
        MSG(RoleCheckBox,        "Kontrollkästchen"),
        MSG(RoleRadioButton,     "Optionsfeld"),
        MSG(RoleComboBox,        "Kombinationsfeld"),
        MSG(RoleListItem,        "Listenelement"),
        MSG(RoleMenu,            "Menü"),
        MSG(RoleMenuItem,        "Menüelement"),
        MSG(RoleDialog,          "Dialogfeld"),
        MSG(RoleWindow,          "Fenster"),
        MSG(StateChecked,        "aktiviert"),
        MSG(StateUnchecked,      "nicht aktiviert"),
        MSG(StateDisabled,       "deaktiviert"),
        MSG(StateSelected,       "ausgewählt"),
        MSG(BrowseModeOn,        "Lesemodus ein"),
        MSG(BrowseModeOff,       "Lesemodus aus"),
        MSG(BrowseModeEndOfDocument, "Ende des Dokuments"),
        MSG(ClipboardEmpty,      "Zwischenablage ist leer"),
        MSG(ScreenReaderStarted, "AccessOS Screenreader gestartet"),
        MSG(ScreenReaderStopped, "AccessOS Screenreader gestoppt"),
    };
}

#undef MSG

} // namespace I18n
} // namespace AccessOS
