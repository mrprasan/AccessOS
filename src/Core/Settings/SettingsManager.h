// AccessOS/src/Core/Settings/SettingsManager.h
//
// SettingsManager — typed facade over ISettingsStore.
//
// Why: Raw key-value access is error-prone. SettingsManager provides
//      strongly-typed properties for every AccessOS setting, with
//      sensible defaults, and centralises all key strings.
//
// Sections:
//   speech.*    — voice, rate, volume, verbosity
//   shortcuts.* — command-id → key binding
//   ui.*        — theme, font size
//   privacy.*   — logging level, secure input filter
//
// Threading: All methods are thread-safe (delegates to ISettingsStore).

#pragma once

#include "ISettingsStore.h"
#include "../Speech/SpeechPolicy.h"

#include <memory>
#include <string>

namespace AccessOS {

class SettingsManager {
public:
    explicit SettingsManager(std::shared_ptr<ISettingsStore> store);

    // ── Open / close ─────────────────────────────────────────────────────────
    bool Open(const std::string& dbPath);
    void Close();
    bool IsOpen() const noexcept;

    // ── Speech settings ──────────────────────────────────────────────────────

    // Voice token ID (SAPI token name). Empty = system default.
    std::string VoiceId() const;
    void        SetVoiceId(const std::string& id);

    // Speech rate: -10 (slowest) to +10 (fastest). Default 0.
    int  SpeechRate() const;
    void SetSpeechRate(int rate);

    // Speech volume: 0–100. Default 100.
    int  SpeechVolume() const;
    void SetSpeechVolume(int volume);

    // Verbosity level: 0=minimal, 1=normal, 2=detailed. Default 1.
    int  Verbosity() const;
    void SetVerbosity(int level);

    // Announce position (e.g. "item 3 of 10"). Default false.
    bool AnnouncePosition() const;
    void SetAnnouncePosition(bool on);

    // Read punctuation: 0=none, 1=some, 2=all. Default 1.
    int  PunctuationLevel() const;
    void SetPunctuationLevel(int level);

    // ── Shortcut bindings ────────────────────────────────────────────────────
    // Stored as "shortcuts.<commandId>" = "<vkCode>:<modifierByte>"

    // Store a shortcut binding for a command ID.
    // Encodes as "<vkCode>:<modifierByte>" (decimal integers).
    void SetShortcut(const std::string& commandId,
                     uint32_t vkCode, uint8_t modifiers);

    // Retrieve the shortcut for a command ID.
    // Returns {0, 0} if not set.
    std::pair<uint32_t, uint8_t> GetShortcut(
        const std::string& commandId) const;

    // Remove a shortcut binding.
    void RemoveShortcut(const std::string& commandId);

    // ── UI settings ──────────────────────────────────────────────────────────

    // "light" or "dark". Default "system".
    std::string Theme() const;
    void        SetTheme(const std::string& theme);

    // ── Privacy settings ─────────────────────────────────────────────────────

    // Whether to suppress speech for password fields. Default true.
    bool SuppressPasswords() const;
    void SetSuppressPasswords(bool on);

    // ── Raw access ───────────────────────────────────────────────────────────
    ISettingsStore& Store() { return *m_store; }

private:
    std::shared_ptr<ISettingsStore> m_store;

    // Type-safe helpers.
    std::string GetStr(const std::string& key,
                       const std::string& def) const;
    int  GetInt(const std::string& key, int def) const;
    bool GetBool(const std::string& key, bool def) const;

    void SetStr(const std::string& key, const std::string& val);
    void SetInt(const std::string& key, int val);
    void SetBool(const std::string& key, bool val);
};

} // namespace AccessOS
