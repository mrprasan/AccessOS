// VoiceProfile.h — Named speech profile store (ACCESSOS-040)
//
// A VoiceProfile bundles a voice ID, speech rate, and volume into a named
// preset that can be saved and loaded via the SettingsManager.
//
// Storage layout (in SQLite settings DB):
//   Key: "voice_profile.<name>.voice_id"  → voice token ID string
//   Key: "voice_profile.<name>.rate"      → integer (-10..+10)
//   Key: "voice_profile.<name>.volume"    → integer (0..100)
//   Key: "voice_profile.active"           → name of the active profile
//   Key: "voice_profile.list"             → comma-separated profile names

#pragma once
#include <string>
#include <vector>
#include <optional>

namespace AccessOS {

// Forward-declared — implementation uses SettingsManager.
// SettingsManager lives directly in namespace AccessOS (not a nested namespace).
class SettingsManager;

namespace Speech {

struct VoiceProfile {
    std::string name;      // unique profile name (e.g. "Default", "FastRead")
    std::string voiceId;   // SAPI voice token ID or display name
    int         rate   = 0;   // -10..+10
    int         volume = 100; // 0..100

    bool operator==(const VoiceProfile& o) const {
        return name == o.name && voiceId == o.voiceId &&
               rate == o.rate && volume == o.volume;
    }
};

class VoiceProfileManager {
public:
    explicit VoiceProfileManager(SettingsManager& settings);

    // ── Profile CRUD ──────────────────────────────────────────────────────────

    // Save a profile (creates or overwrites).
    void Save(const VoiceProfile& profile);

    // Load a profile by name. Returns nullopt if not found.
    std::optional<VoiceProfile> Load(const std::string& name) const;

    // Delete a profile by name. Returns true if it existed.
    bool Delete(const std::string& name);

    // Returns all stored profile names.
    std::vector<std::string> ListNames() const;

    // ── Active profile ────────────────────────────────────────────────────────

    // Set the active profile name (persisted).
    void SetActive(const std::string& name);

    // Returns the active profile name, or empty string if none.
    std::string GetActive() const;

    // Load the active profile. Returns nullopt if none is set or not found.
    std::optional<VoiceProfile> LoadActive() const;

    // ── Built-in profiles ─────────────────────────────────────────────────────

    // Ensure the "Default" profile exists (creates it if absent).
    // Returns the Default profile.
    VoiceProfile EnsureDefault();

private:
    SettingsManager& m_settings;

    // Key helpers
    static std::string KeyVoice (const std::string& name);
    static std::string KeyRate  (const std::string& name);
    static std::string KeyVolume(const std::string& name);
    static std::string KeyList  ();
    static std::string KeyActive();

    // Append name to the comma-separated list if not already present.
    void AddToList(const std::string& name);

    // Remove name from the comma-separated list.
    void RemoveFromList(const std::string& name);
};

} // namespace Speech
} // namespace AccessOS
