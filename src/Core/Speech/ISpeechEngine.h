// AccessOS/src/Core/Speech/ISpeechEngine.h
//
// Speech engine interface — abstracts TTS backends.
//
// Why: The architecture requires that speech logic never depend on a
//      specific TTS implementation. SAPI is the initial backend, but
//      additional engines (OneCore, third-party) must be addable without
//      changing any consuming code.
//
// Threading: All methods must be called from the SpeechManager's
//            dedicated speech thread only.

#pragma once

#include "../Error/AccessError.h"
#include <string>
#include <vector>

namespace AccessOS {

// Voice descriptor — engine-independent voice representation.
struct VoiceInfo {
    std::string id;       // Engine-specific identifier
    std::string name;     // Display name (e.g. "Microsoft David")
    std::string language; // BCP-47 language tag (e.g. "en-US")
};

// ─── ISpeechEngine ────────────────────────────────────────────────────────────
class ISpeechEngine {
public:
    virtual ~ISpeechEngine() = default;

    // Initialize the engine. Must be called before any other method.
    virtual Result<void> Initialize() = 0;

    // Release engine resources.
    virtual void Shutdown() = 0;

    // Returns true if the engine is ready to speak.
    virtual bool IsAvailable() const noexcept = 0;

    // Speak text synchronously — blocks until utterance completes or is stopped.
    // Called only from the speech thread.
    virtual Result<void> Speak(const std::string& text) = 0;

    // Stop the current utterance immediately.
    virtual void Stop() = 0;

    // Pause the current utterance.
    virtual void Pause() = 0;

    // Resume a paused utterance.
    virtual void Resume() = 0;

    // Set speech rate. Range: -10 (slowest) to +10 (fastest). Default: 0.
    virtual Result<void> SetRate(int rate) = 0;

    // Set speech volume. Range: 0–100. Default: 100.
    virtual Result<void> SetVolume(int volume) = 0;

    // Set the active voice by ID. Returns error if voice not found.
    virtual Result<void> SetVoice(const std::string& voiceId) = 0;

    // Returns available voices on this system.
    virtual std::vector<VoiceInfo> GetAvailableVoices() const = 0;

    // Returns the engine display name for diagnostics.
    virtual const char* EngineName() const noexcept = 0;
};

} // namespace AccessOS
