#pragma once
// IAudioEngine.h — Abstract audio engine interface (ACCESSOS-024)

#include "EarconId.h"
#include <cstdint>

namespace AccessOS {
namespace Audio {

// Parameters for a synthesised sine-tone earcon
struct ToneParams {
    float    frequencyHz  = 440.0f;  // fundamental frequency
    float    durationMs   = 80.0f;   // tone length in milliseconds
    float    volume       = 0.7f;    // 0.0–1.0
    float    fadeInMs     = 5.0f;    // attack ramp (avoids click)
    float    fadeOutMs    = 10.0f;   // release ramp
};

class IAudioEngine {
public:
    virtual ~IAudioEngine() = default;

    // True if audio output is available on this system
    virtual bool IsAvailable() const = 0;

    // Play a synthesised sine tone with the given parameters (non-blocking)
    // Returns false if unavailable or muted
    virtual bool PlayTone(const ToneParams& params) = 0;

    // Set master output volume (0.0–1.0); clamped silently
    virtual void SetVolume(float volume) = 0;
    virtual float GetVolume() const = 0;

    // Mute / unmute: PlayTone returns true but produces no sound when muted
    virtual void SetMuted(bool muted) = 0;
    virtual bool IsMuted() const = 0;
};

} // namespace Audio
} // namespace AccessOS
