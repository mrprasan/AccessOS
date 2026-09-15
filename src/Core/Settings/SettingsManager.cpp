// AccessOS/src/Core/Settings/SettingsManager.cpp

#include "SettingsManager.h"

#include <string>
#include <stdexcept>
#include <charconv>
#include <sstream>

namespace AccessOS {

// ── Key constants ─────────────────────────────────────────────────────────────

static constexpr const char* kVoiceId           = "speech.voiceId";
static constexpr const char* kSpeechRate        = "speech.rate";
static constexpr const char* kSpeechVolume      = "speech.volume";
static constexpr const char* kVerbosity         = "speech.verbosity";
static constexpr const char* kAnnouncePosition  = "speech.announcePosition";
static constexpr const char* kPunctuationLevel  = "speech.punctuationLevel";
static constexpr const char* kTheme             = "ui.theme";
static constexpr const char* kSuppressPasswords = "privacy.suppressPasswords";

// ── Construction ─────────────────────────────────────────────────────────────

SettingsManager::SettingsManager(std::shared_ptr<ISettingsStore> store)
    : m_store(std::move(store))
{
}

// ── Open / Close ──────────────────────────────────────────────────────────────

bool SettingsManager::Open(const std::string& dbPath) {
    return m_store->Open(dbPath);
}

void SettingsManager::Close() {
    m_store->Close();
}

bool SettingsManager::IsOpen() const noexcept {
    return m_store->IsOpen();
}

// ── Speech ────────────────────────────────────────────────────────────────────

std::string SettingsManager::VoiceId() const {
    return GetStr(kVoiceId, "");
}
void SettingsManager::SetVoiceId(const std::string& id) {
    SetStr(kVoiceId, id);
}

int SettingsManager::SpeechRate() const {
    return GetInt(kSpeechRate, 0);
}
void SettingsManager::SetSpeechRate(int rate) {
    SetInt(kSpeechRate, rate);
}

int SettingsManager::SpeechVolume() const {
    return GetInt(kSpeechVolume, 100);
}
void SettingsManager::SetSpeechVolume(int volume) {
    SetInt(kSpeechVolume, volume);
}

int SettingsManager::Verbosity() const {
    return GetInt(kVerbosity, 1);
}
void SettingsManager::SetVerbosity(int level) {
    SetInt(kVerbosity, level);
}

bool SettingsManager::AnnouncePosition() const {
    return GetBool(kAnnouncePosition, false);
}
void SettingsManager::SetAnnouncePosition(bool on) {
    SetBool(kAnnouncePosition, on);
}

int SettingsManager::PunctuationLevel() const {
    return GetInt(kPunctuationLevel, 1);
}
void SettingsManager::SetPunctuationLevel(int level) {
    SetInt(kPunctuationLevel, level);
}

// ── Shortcuts ─────────────────────────────────────────────────────────────────

void SettingsManager::SetShortcut(const std::string& commandId,
                                   uint32_t vkCode, uint8_t modifiers)
{
    const std::string key = "shortcuts." + commandId;
    const std::string val = std::to_string(vkCode) + ":" +
                            std::to_string(static_cast<int>(modifiers));
    SetStr(key, val);
}

std::pair<uint32_t, uint8_t> SettingsManager::GetShortcut(
    const std::string& commandId) const
{
    const std::string key = "shortcuts." + commandId;
    auto raw = m_store->Get(key);
    if (!raw.has_value()) return std::pair<uint32_t, uint8_t>{0u, uint8_t(0)};

    // Parse "<vkCode>:<modifierByte>"
    const std::string& s = *raw;
    const size_t colon = s.find(':');
    if (colon == std::string::npos) return std::pair<uint32_t, uint8_t>{0u, uint8_t(0)};

    uint32_t vk = 0;
    uint8_t  mod = 0;
    std::from_chars(s.data(), s.data() + colon, vk);
    int modInt = 0;
    std::from_chars(s.data() + colon + 1, s.data() + s.size(), modInt);
    mod = static_cast<uint8_t>(modInt);
    return std::pair<uint32_t, uint8_t>{vk, mod};
}

void SettingsManager::RemoveShortcut(const std::string& commandId) {
    m_store->Remove("shortcuts." + commandId);
}

// ── UI ────────────────────────────────────────────────────────────────────────

std::string SettingsManager::Theme() const {
    return GetStr(kTheme, "system");
}
void SettingsManager::SetTheme(const std::string& theme) {
    SetStr(kTheme, theme);
}

// ── Privacy ───────────────────────────────────────────────────────────────────

bool SettingsManager::SuppressPasswords() const {
    return GetBool(kSuppressPasswords, true);
}
void SettingsManager::SetSuppressPasswords(bool on) {
    SetBool(kSuppressPasswords, on);
}

// ── Private helpers ───────────────────────────────────────────────────────────

std::string SettingsManager::GetStr(const std::string& key,
                                     const std::string& def) const {
    return m_store->GetOr(key, def);
}

int SettingsManager::GetInt(const std::string& key, int def) const {
    auto v = m_store->Get(key);
    if (!v.has_value()) return def;
    int result = def;
    std::from_chars(v->data(), v->data() + v->size(), result);
    return result;
}

bool SettingsManager::GetBool(const std::string& key, bool def) const {
    auto v = m_store->Get(key);
    if (!v.has_value()) return def;
    return (*v == "1" || *v == "true");
}

void SettingsManager::SetStr(const std::string& key, const std::string& val) {
    m_store->Set(key, val);
}

void SettingsManager::SetInt(const std::string& key, int val) {
    m_store->Set(key, std::to_string(val));
}

void SettingsManager::SetBool(const std::string& key, bool val) {
    m_store->Set(key, val ? "1" : "0");
}

} // namespace AccessOS
