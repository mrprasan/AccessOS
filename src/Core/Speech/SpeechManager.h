// AccessOS/src/Core/Speech/SpeechManager.h
//
// SpeechManager — owns the speech thread, the speech queue, and the
// speech engine. Provides the public API for all speech operations.
//
// Why: All speech must be serialized on a dedicated thread. No other
//      subsystem should call ISpeechEngine directly. SpeechManager
//      is the single entry point for speech output.
//
// Threading:
//   - Speak(), Stop(), Cancel(), etc. are thread-safe.
//     They enqueue requests; the speech thread executes them.
//   - The speech thread is owned entirely by SpeechManager.
//   - ISpeechEngine methods are called only from the speech thread.

#pragma once

#include "ISpeechEngine.h"
#include "SpeechQueue.h"
#include "SpeechFormatter.h"
#include "SpeechPolicy.h"
#include "../Semantic/AccessNode.h"
#include "../Error/AccessError.h"

#include <memory>
#include <thread>
#include <atomic>

namespace AccessOS {

class SpeechManager {
public:
    SpeechManager();
    ~SpeechManager();

    // Initialize with a specific engine. Takes ownership of the engine.
    Result<void> Initialize(std::unique_ptr<ISpeechEngine> engine);

    // Shut down the speech thread and engine cleanly.
    void Shutdown();

    bool IsRunning() const noexcept { return m_running.load(); }

    // Speak an AccessNode — formats via SpeechFormatter then enqueues.
    void SpeakNode(const AccessNode& node,
                   SpeechPriority priority = SpeechPriority::Normal);

    // Speak a plain text string.
    void SpeakText(const std::string& text,
                   SpeechPriority priority = SpeechPriority::Normal,
                   bool cancelPrevious = false);

    // Stop current utterance and clear the queue.
    void Stop();

    // Cancel all Normal and Low speech (called on navigation).
    void CancelStaleSpeech();

    // Pause the current utterance.
    void Pause();

    // Resume a paused utterance.
    void Resume();

    // Set speech rate (-10 to +10). Applied on next utterance.
    void SetRate(int rate);

    // Set speech volume (0–100). Applied on next utterance.
    void SetVolume(int volume);

    // Set active voice by ID.
    void SetVoice(const std::string& voiceId);

    // Replace the active speech policy.
    void SetPolicy(SpeechPolicy policy);

    // Get the current speech policy.
    SpeechPolicy GetPolicy() const;

private:
    void SpeechThread();
    void ApplyPendingSettings();

    std::unique_ptr<ISpeechEngine>  m_engine;
    std::unique_ptr<SpeechQueue>    m_queue;

    std::thread                     m_thread;
    std::atomic<bool>               m_running{ false };

    // Pending settings — applied on the speech thread before next utterance.
    mutable std::mutex  m_settingsMutex;
    SpeechPolicy        m_policy;
    int                 m_pendingRate{ 0 };
    int                 m_pendingVolume{ 100 };
    std::string         m_pendingVoiceId;
    bool                m_settingsDirty{ false };
};

} // namespace AccessOS
