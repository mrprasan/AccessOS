// AccessOS/src/Core/Speech/SpeechManager.cpp

// COM headers must be explicit — SpeechManager.h does not pull in windows.h.
#include <objbase.h>   // CoInitializeEx, CoUninitialize, COINIT_APARTMENTTHREADED
#include <windows.h>
#include "SpeechManager.h"
#include "../Logging/Logger.h"

namespace AccessOS {

static constexpr const char* kComponent = "SpeechManager";

SpeechManager::SpeechManager()
    : m_queue(std::make_unique<SpeechQueue>())
{
}

SpeechManager::~SpeechManager() {
    Shutdown();
}

Result<void> SpeechManager::Initialize(std::unique_ptr<ISpeechEngine> engine) {
    if (!engine) {
        return Result<void>::Fail(
            MakeError(ErrorCode::SpeechEngineUnavailable),
            "Null engine passed to SpeechManager::Initialize");
    }

    m_engine = std::move(engine);

    // Engine initialization happens on the speech thread so COM apartment
    // requirements are met on the correct thread.
    m_running.store(true);
    m_thread = std::thread(&SpeechManager::SpeechThread, this);

    ACOS_LOG_INFO(kComponent, "SpeechManager initialized — speech thread started");
    return Result<void>::Ok();
}

void SpeechManager::Shutdown() {
    if (!m_running.exchange(false)) return;

    ACOS_LOG_INFO(kComponent, "Shutting down SpeechManager");
    m_queue->Stop();

    if (m_thread.joinable()) {
        m_thread.join();
    }

    ACOS_LOG_INFO(kComponent, "SpeechManager shut down");
}

void SpeechManager::SpeakNode(const AccessNode& node, SpeechPriority priority) {
    SpeechPolicy policy;
    {
        std::lock_guard<std::mutex> lock(m_settingsMutex);
        policy = m_policy;
    }

    // Protected elements: never speak value content.
    // The formatter enforces this too, but we guard here as well.
    std::string text = SpeechFormatter::Format(node, policy);
    if (text.empty()) return;

    SpeakText(text, priority,
              policy.navigationCancelsNormal &&
              priority >= SpeechPriority::Normal);
}

void SpeechManager::SpeakText(const std::string& text,
                               SpeechPriority priority,
                               bool cancelPrevious)
{
    if (!m_running.load() || text.empty()) return;

    SpeechRequest req;
    req.text           = text;
    req.priority       = priority;
    req.cancelPrevious = cancelPrevious;
    req.id             = m_queue->NextId();

    m_queue->Enqueue(std::move(req));
}

void SpeechManager::Stop() {
    if (!m_running.load()) return;
    m_queue->CancelAll();
    if (m_engine && m_engine->IsAvailable()) {
        m_engine->Stop();
    }
}

void SpeechManager::CancelStaleSpeech() {
    // Cancel all Normal and Low priority speech — called when user navigates.
    m_queue->CancelUpTo(SpeechPriority::Normal);
    if (m_engine && m_engine->IsAvailable()) {
        m_engine->Stop();
    }
}

void SpeechManager::Pause() {
    if (m_engine && m_engine->IsAvailable()) m_engine->Pause();
}

void SpeechManager::Resume() {
    if (m_engine && m_engine->IsAvailable()) m_engine->Resume();
}

void SpeechManager::SetRate(int rate) {
    std::lock_guard<std::mutex> lock(m_settingsMutex);
    m_pendingRate    = rate;
    m_settingsDirty  = true;
}

void SpeechManager::SetVolume(int volume) {
    std::lock_guard<std::mutex> lock(m_settingsMutex);
    m_pendingVolume  = volume;
    m_settingsDirty  = true;
}

void SpeechManager::SetVoice(const std::string& voiceId) {
    std::lock_guard<std::mutex> lock(m_settingsMutex);
    m_pendingVoiceId = voiceId;
    m_settingsDirty  = true;
}

void SpeechManager::SetPolicy(SpeechPolicy policy) {
    std::lock_guard<std::mutex> lock(m_settingsMutex);
    m_policy = policy;
}

SpeechPolicy SpeechManager::GetPolicy() const {
    std::lock_guard<std::mutex> lock(m_settingsMutex);
    return m_policy;
}

// ─── Speech thread ────────────────────────────────────────────────────────────

void SpeechManager::SpeechThread() {
    ACOS_LOG_INFO(kComponent, "Speech thread started");

    // Initialize COM on the speech thread (STA for SAPI compatibility).
    HRESULT hr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool comInitialized = SUCCEEDED(hr);
    if (!comInitialized) {
        ACOS_LOG_CRITICAL(kComponent, "CoInitializeEx failed on speech thread");
        m_running.store(false);
        return;
    }

    // Initialize the engine on this thread.
    if (m_engine) {
        auto result = m_engine->Initialize();
        if (result.IsError()) {
            ACOS_LOG_CRITICAL(kComponent,
                "Speech engine initialization failed: " + result.Message());
            ::CoUninitialize();
            m_running.store(false);
            return;
        }
    }

    // Drain the queue until stopped.
    while (!m_queue->IsStopped()) {
        ApplyPendingSettings();

        auto req = m_queue->Dequeue(std::chrono::milliseconds(100));
        if (!req.has_value()) continue;

        ACOS_LOG_DEBUG(kComponent, "Speaking: " + req->text);

        if (m_engine && m_engine->IsAvailable()) {
            auto result = m_engine->Speak(req->text);
            if (result.IsError()) {
                ACOS_LOG_ERROR(kComponent,
                    "Speak failed: " + result.Message());
            }
        }
    }

    if (m_engine) m_engine->Shutdown();
    if (comInitialized) ::CoUninitialize();

    ACOS_LOG_INFO(kComponent, "Speech thread exiting");
}

void SpeechManager::ApplyPendingSettings() {
    std::lock_guard<std::mutex> lock(m_settingsMutex);
    if (!m_settingsDirty || !m_engine || !m_engine->IsAvailable()) return;

    if (m_pendingRate != 0)       m_engine->SetRate(m_pendingRate);
    if (m_pendingVolume != 100)   m_engine->SetVolume(m_pendingVolume);
    if (!m_pendingVoiceId.empty()) m_engine->SetVoice(m_pendingVoiceId);

    m_settingsDirty = false;
}

} // namespace AccessOS
