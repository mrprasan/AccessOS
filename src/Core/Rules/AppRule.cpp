// AppRule.cpp — Per-application script rules (ACCESSOS-042)

#include "AppRule.h"
#include "../Settings/SettingsManager.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace AccessOS {
namespace Rules {

// ── key helpers ───────────────────────────────────────────────────────────────

std::string AppRuleManager::Normalize(const std::string& exe) {
    std::string out = exe;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

std::string AppRuleManager::KeyList() {
    return "apprule.list";
}
std::string AppRuleManager::KeyVerbosity(const std::string& exe) {
    return "apprule." + exe + ".verbosity";
}
std::string AppRuleManager::KeyBrowseAuto(const std::string& exe) {
    return "apprule." + exe + ".browse_auto";
}
std::string AppRuleManager::KeyEarcons(const std::string& exe) {
    return "apprule." + exe + ".earcons";
}
std::string AppRuleManager::KeyRoleTemplates(const std::string& exe) {
    return "apprule." + exe + ".role_templates";
}
std::string AppRuleManager::KeyExtra(const std::string& exe) {
    return "apprule." + exe + ".extra";
}

// ── construction ──────────────────────────────────────────────────────────────

AppRuleManager::AppRuleManager(SettingsManager& settings)
    : m_settings(settings) {}

// ── CRUD ──────────────────────────────────────────────────────────────────────

void AppRuleManager::Save(const AppRule& rule) {
    if (rule.exeName.empty()) return;
    std::string exe = Normalize(rule.exeName);

    if (rule.verbosity.has_value())
        m_settings.Store().Set(KeyVerbosity(exe), std::to_string(*rule.verbosity));
    else
        m_settings.Store().Remove(KeyVerbosity(exe));

    if (rule.browseModeAuto.has_value())
        m_settings.Store().Set(KeyBrowseAuto(exe), *rule.browseModeAuto ? "1" : "0");
    else
        m_settings.Store().Remove(KeyBrowseAuto(exe));

    if (rule.earconsEnabled.has_value())
        m_settings.Store().Set(KeyEarcons(exe), *rule.earconsEnabled ? "1" : "0");
    else
        m_settings.Store().Remove(KeyEarcons(exe));

    if (!rule.roleTemplates.empty())
        m_settings.Store().Set(KeyRoleTemplates(exe), EncodeMap(rule.roleTemplates));
    else
        m_settings.Store().Remove(KeyRoleTemplates(exe));

    if (!rule.extra.empty())
        m_settings.Store().Set(KeyExtra(exe), EncodeMap(rule.extra));
    else
        m_settings.Store().Remove(KeyExtra(exe));

    // Always record in list so we know it exists (even if all fields are null)
    AddToList(exe);
    // Ensure the record anchor exists
    m_settings.Store().Set("apprule." + exe + ".exists", "1");
}

std::optional<AppRule> AppRuleManager::Get(const std::string& exeName) const {
    if (exeName.empty()) return std::nullopt;
    std::string exe = Normalize(exeName);

    // Check existence
    auto exists = m_settings.Store().Get("apprule." + exe + ".exists");
    if (!exists.has_value()) return std::nullopt;

    AppRule rule;
    rule.exeName = exe;

    auto verbStr = m_settings.Store().Get(KeyVerbosity(exe));
    if (verbStr.has_value() && !verbStr->empty()) {
        try { rule.verbosity = std::stoi(*verbStr); } catch (...) {}
    }

    auto browseStr = m_settings.Store().Get(KeyBrowseAuto(exe));
    if (browseStr.has_value()) rule.browseModeAuto = (*browseStr == "1");

    auto earconStr = m_settings.Store().Get(KeyEarcons(exe));
    if (earconStr.has_value()) rule.earconsEnabled = (*earconStr == "1");

    auto roleStr = m_settings.Store().Get(KeyRoleTemplates(exe));
    if (roleStr.has_value() && !roleStr->empty())
        rule.roleTemplates = DecodeMap(*roleStr);

    auto extraStr = m_settings.Store().Get(KeyExtra(exe));
    if (extraStr.has_value() && !extraStr->empty())
        rule.extra = DecodeMap(*extraStr);

    return rule;
}

bool AppRuleManager::Delete(const std::string& exeName) {
    if (exeName.empty()) return false;
    std::string exe = Normalize(exeName);

    auto exists = m_settings.Store().Get("apprule." + exe + ".exists");
    if (!exists.has_value()) return false;

    m_settings.Store().Remove("apprule." + exe + ".exists");
    m_settings.Store().Remove(KeyVerbosity(exe));
    m_settings.Store().Remove(KeyBrowseAuto(exe));
    m_settings.Store().Remove(KeyEarcons(exe));
    m_settings.Store().Remove(KeyRoleTemplates(exe));
    m_settings.Store().Remove(KeyExtra(exe));
    RemoveFromList(exe);
    return true;
}

std::vector<std::string> AppRuleManager::ListExeNames() const {
    auto raw = m_settings.Store().Get(KeyList());
    if (!raw.has_value() || raw->empty()) return {};
    std::vector<std::string> names;
    std::istringstream ss(*raw);
    std::string token;
    while (std::getline(ss, token, ','))
        if (!token.empty()) names.push_back(token);
    return names;
}

// ── Per-field update helpers ──────────────────────────────────────────────────

void AppRuleManager::SetVerbosity(const std::string& exeName, int level) {
    auto rule = Get(exeName).value_or(AppRule{});
    rule.exeName   = Normalize(exeName);
    rule.verbosity = level;
    Save(rule);
}

void AppRuleManager::SetBrowseModeAuto(const std::string& exeName, bool on) {
    auto rule = Get(exeName).value_or(AppRule{});
    rule.exeName       = Normalize(exeName);
    rule.browseModeAuto= on;
    Save(rule);
}

void AppRuleManager::SetEarconsEnabled(const std::string& exeName, bool on) {
    auto rule = Get(exeName).value_or(AppRule{});
    rule.exeName       = Normalize(exeName);
    rule.earconsEnabled= on;
    Save(rule);
}

void AppRuleManager::SetRoleTemplate(const std::string& exeName,
                                      const std::string& role,
                                      const std::string& tmpl) {
    auto rule = Get(exeName).value_or(AppRule{});
    rule.exeName = Normalize(exeName);
    rule.roleTemplates[role] = tmpl;
    Save(rule);
}

void AppRuleManager::RemoveRoleTemplate(const std::string& exeName,
                                         const std::string& role) {
    auto rule = Get(exeName);
    if (!rule.has_value()) return;
    rule->roleTemplates.erase(role);
    Save(*rule);
}

// ── Matching ──────────────────────────────────────────────────────────────────

std::optional<AppRule> AppRuleManager::Match(const std::string& exeName) const {
    return Get(exeName); // Get already normalises
}

// ── private helpers ───────────────────────────────────────────────────────────

void AppRuleManager::AddToList(const std::string& exe) {
    auto names = ListExeNames();
    if (std::find(names.begin(), names.end(), exe) != names.end()) return;
    names.push_back(exe);
    std::string joined;
    for (size_t i = 0; i < names.size(); ++i) {
        if (i > 0) joined += ',';
        joined += names[i];
    }
    m_settings.Store().Set(KeyList(), joined);
}

void AppRuleManager::RemoveFromList(const std::string& exe) {
    auto names = ListExeNames();
    names.erase(std::remove(names.begin(), names.end(), exe), names.end());
    std::string joined;
    for (size_t i = 0; i < names.size(); ++i) {
        if (i > 0) joined += ',';
        joined += names[i];
    }
    m_settings.Store().Set(KeyList(), joined);
}

std::string AppRuleManager::EncodeMap(
        const std::unordered_map<std::string,std::string>& m) {
    // "k1=v1;k2=v2" — keys and values must not contain '=' or ';'
    std::string out;
    for (auto& [k, v] : m) {
        if (!out.empty()) out += ';';
        out += k + '=' + v;
    }
    return out;
}

std::unordered_map<std::string,std::string>
AppRuleManager::DecodeMap(const std::string& s) {
    std::unordered_map<std::string,std::string> out;
    std::istringstream ss(s);
    std::string pair;
    while (std::getline(ss, pair, ';')) {
        auto eq = pair.find('=');
        if (eq == std::string::npos) continue;
        out[pair.substr(0, eq)] = pair.substr(eq + 1);
    }
    return out;
}

} // namespace Rules
} // namespace AccessOS
