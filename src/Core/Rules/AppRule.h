// AppRule.h — Per-application script rules (ACCESSOS-042)
//
// Stores named rule sets that override AccessOS behaviour for specific
// applications. Rules are matched by process executable name (case-insensitive).
//
// Each rule set can override:
//   • verbosity level (0=minimal … 3=developer)
//   • whether browse mode auto-activates
//   • a custom role→announcement template (e.g. redefine how "button" is read)
//   • earcon enable/disable per earcon ID
//   • a set of free-form key=value extension pairs for future use
//
// Storage: rules are persisted via SettingsManager under the
//   "apprule.<exe>.*" key namespace.
//
// Matching: GetRule("notepad.exe") returns the rule for that exe if one exists.
//           GetRule with an empty exe returns nullopt.

#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <vector>

namespace AccessOS {

// SettingsManager lives directly in namespace AccessOS.
class SettingsManager;

namespace Rules {

struct AppRule {
    std::string exeName;            // e.g. "notepad.exe" (lower-case)

    // Overrides — std::nullopt means "use global default"
    std::optional<int>  verbosity;          // 0..3
    std::optional<bool> browseModeAuto;     // true = auto-enter browse mode
    std::optional<bool> earconsEnabled;     // master earcon enable for this app

    // Per-role announcement template overrides.
    // Key: role string (e.g. "button"), Value: template (e.g. "{name}, tap")
    std::unordered_map<std::string, std::string> roleTemplates;

    // Free-form key-value extensions (for future script hooks).
    std::unordered_map<std::string, std::string> extra;
};

class AppRuleManager {
public:
    explicit AppRuleManager(SettingsManager& settings);

    // ── CRUD ──────────────────────────────────────────────────────────────────

    // Save (or overwrite) a rule.
    void Save(const AppRule& rule);

    // Load the rule for an exe name. Returns nullopt if not found.
    std::optional<AppRule> Get(const std::string& exeName) const;

    // Delete the rule for an exe. Returns true if it existed.
    bool Delete(const std::string& exeName);

    // Returns all registered exe names.
    std::vector<std::string> ListExeNames() const;

    // ── Per-field update helpers ──────────────────────────────────────────────

    void SetVerbosity     (const std::string& exeName, int level);
    void SetBrowseModeAuto(const std::string& exeName, bool on);
    void SetEarconsEnabled(const std::string& exeName, bool on);
    void SetRoleTemplate  (const std::string& exeName,
                           const std::string& role,
                           const std::string& tmpl);
    void RemoveRoleTemplate(const std::string& exeName, const std::string& role);

    // ── Matching ──────────────────────────────────────────────────────────────

    // Case-insensitive lookup. Returns nullopt if exeName is empty or no match.
    std::optional<AppRule> Match(const std::string& exeName) const;

private:
    SettingsManager& m_settings;

    static std::string Normalize(const std::string& exe);
    static std::string KeyList();
    static std::string KeyVerbosity     (const std::string& exe);
    static std::string KeyBrowseAuto    (const std::string& exe);
    static std::string KeyEarcons       (const std::string& exe);
    static std::string KeyRoleTemplates (const std::string& exe);
    static std::string KeyExtra         (const std::string& exe);

    void AddToList   (const std::string& exe);
    void RemoveFromList(const std::string& exe);

    // Encode/decode map<string,string> as "k1=v1;k2=v2"
    static std::string EncodeMap(const std::unordered_map<std::string,std::string>& m);
    static std::unordered_map<std::string,std::string> DecodeMap(const std::string& s);
};

} // namespace Rules
} // namespace AccessOS
