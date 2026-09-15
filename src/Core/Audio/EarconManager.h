#pragma once
// EarconManager.h — maps EarconId → ToneParams, drives IAudioEngine (ACCESSOS-024)
//
// Responsibilities:
//   - Default tone table for all EarconId values
//   - Per-earcon enable/disable
//   - Master enable/disable and volume
//   - Thread-safe Play() entry point

#include "EarconId.h"
#include "IAudioEngine.h"
#include <memory>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace AccessOS {
namespace Audio {

class EarconManager {
public:
    EarconManager();
    explicit EarconManager(std::shared_ptr<IAudioEngine> engine);

    // Replace the audio engine (resets nothing else)
    void SetEngine(std::shared_ptr<IAudioEngine> engine);
    std::shared_ptr<IAudioEngine> GetEngine() const;

    bool IsAvailable() const;

    // Play the earcon for the given id.
    // Returns false if disabled, muted, engine unavailable, or id is None.
    bool Play(EarconId id);

    // Override tone params for a specific earcon
    void SetToneParams(EarconId id, const ToneParams& params);
    ToneParams GetToneParams(EarconId id) const;

    // Per-earcon enable / disable
    void SetEarconEnabled(EarconId id, bool enabled);
    bool IsEarconEnabled(EarconId id) const;

    // Master enable / disable (overrides per-earcon)
    void SetEnabled(bool enabled);
    bool IsEnabled() const;

    // Master volume (forwarded to engine)
    void SetVolume(float volume);
    float GetVolume() const;

    // Mute (forwarded to engine)
    void SetMuted(bool muted);
    bool IsMuted() const;

private:
    std::shared_ptr<IAudioEngine>                      m_engine;
    std::unordered_map<uint32_t, ToneParams>           m_toneTable;
    std::unordered_map<uint32_t, bool>                 m_enabledMap;
    std::atomic<bool>                                  m_masterEnabled{ true };
    mutable std::mutex                                 m_mutex;

    void BuildDefaultTable();
    static ToneParams DefaultTone(EarconId id);
};

} // namespace Audio
} // namespace AccessOS
