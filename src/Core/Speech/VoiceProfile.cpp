// VoiceProfile.cpp — Named speech profile store (ACCESSOS-040)

#include "VoiceProfile.h"
#include "../Settings/SettingsManager.h"

#include <sstream>
#include <algorithm>

namespace AccessOS {
namespace Speech {

// ── key helpers ───────────────────────────────────────────────────────────────

static std::string PfxProfile(const std::string& name) {
    return "voice_profile." + name;
}

std::string VoiceProfileManager::KeyVoice (const std::string& name) {
    return PfxProfile(name) + ".voice_id";
}
std::string VoiceProfileManager::KeyRate  (const std::string& name) {
    return PfxProfile(name) + ".rate";
}
std::string VoiceProfileManager::KeyVolume(const std::string& name) {
    return PfxProfile(name) + ".volume";
}
std::string VoiceProfileManager::KeyList() {
    return "voice_profile.list";
}
std::string VoiceProfileManager::KeyActive() {
    return "voice_profile.active";
}

// ── construction ──────────────────────────────────────────────────────────────

VoiceProfileManager::VoiceProfileManager(SettingsManager& settings)
    : m_settings(settings) {}

// ── Profile CRUD ──────────────────────────────────────────────────────────────

void VoiceProfileManager::Save(const VoiceProfile& profile) {
    if (profile.name.empty()) return;

    m_settings.Store().Set(KeyVoice (profile.name), profile.voiceId);
    m_settings.Store().Set(KeyRate  (profile.name), std::to_string(profile.rate));
    m_settings.Store().Set(KeyVolume(profile.name), std::to_string(profile.volume));
    AddToList(profile.name);
}

std::optional<VoiceProfile> VoiceProfileManager::Load(const std::string& name) const {
    if (name.empty()) return std::nullopt;

    // Check existence: voice_id key must be present
    auto voiceVal = m_settings.Store().Get(KeyVoice(name));
    if (!voiceVal.has_value()) return std::nullopt;

    VoiceProfile p;
    p.name    = name;
    p.voiceId = voiceVal.value_or("");

    auto rateStr = m_settings.Store().Get(KeyRate(name));
    auto volStr  = m_settings.Store().Get(KeyVolume(name));

    try {
        if (rateStr.has_value() && !rateStr->empty())
            p.rate = std::stoi(*rateStr);
    } catch (...) { p.rate = 0; }

    try {
        if (volStr.has_value() && !volStr->empty())
            p.volume = std::stoi(*volStr);
    } catch (...) { p.volume = 100; }

    return p;
}

bool VoiceProfileManager::Delete(const std::string& name) {
    if (name.empty()) return false;

    // Check it exists
    auto exists = m_settings.Store().Get(KeyVoice(name));
    if (!exists.has_value()) return false;

    m_settings.Store().Remove(KeyVoice (name));
    m_settings.Store().Remove(KeyRate  (name));
    m_settings.Store().Remove(KeyVolume(name));
    RemoveFromList(name);

    // If this was the active profile, clear active
    auto active = m_settings.Store().Get(KeyActive());
    if (active.has_value() && *active == name) {
        m_settings.Store().Remove(KeyActive());
    }
    return true;
}

std::vector<std::string> VoiceProfileManager::ListNames() const {
    auto raw = m_settings.Store().Get(KeyList());
    if (!raw.has_value() || raw->empty()) return {};

    std::vector<std::string> names;
    std::istringstream ss(*raw);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if (!token.empty()) names.push_back(token);
    }
    return names;
}

// ── Active profile ────────────────────────────────────────────────────────────

void VoiceProfileManager::SetActive(const std::string& name) {
    m_settings.Store().Set(KeyActive(), name);
}

std::string VoiceProfileManager::GetActive() const {
    auto v = m_settings.Store().Get(KeyActive());
    return v.value_or("");
}

std::optional<VoiceProfile> VoiceProfileManager::LoadActive() const {
    std::string active = GetActive();
    if (active.empty()) return std::nullopt;
    return Load(active);
}

// ── Built-in profiles ─────────────────────────────────────────────────────────

VoiceProfile VoiceProfileManager::EnsureDefault() {
    auto existing = Load("Default");
    if (existing.has_value()) return *existing;

    // Create default from current speech settings
    VoiceProfile def;
    def.name    = "Default";
    def.voiceId = m_settings.VoiceId();
    def.rate    = m_settings.SpeechRate();
    def.volume  = m_settings.SpeechVolume();
    Save(def);
    return def;
}

// ── private helpers ───────────────────────────────────────────────────────────

void VoiceProfileManager::AddToList(const std::string& name) {
    auto names = ListNames();
    if (std::find(names.begin(), names.end(), name) != names.end()) return;
    names.push_back(name);

    std::string joined;
    for (size_t i = 0; i < names.size(); ++i) {
        if (i > 0) joined += ',';
        joined += names[i];
    }
    m_settings.Store().Set(KeyList(), joined);
}

void VoiceProfileManager::RemoveFromList(const std::string& name) {
    auto names = ListNames();
    names.erase(std::remove(names.begin(), names.end(), name), names.end());

    std::string joined;
    for (size_t i = 0; i < names.size(); ++i) {
        if (i > 0) joined += ',';
        joined += names[i];
    }
    m_settings.Store().Set(KeyList(), joined);
}

} // namespace Speech
} // namespace AccessOS
