// AccessOS/src/Core/Speech/SapiSpeechEngine.cpp
// Note: sphelper.h (ATL-dependent) is intentionally excluded.
// Voice category access uses direct COM enumeration via ISpObjectTokenCategory.

#include "SapiSpeechEngine.h"
#include "../Logging/Logger.h"

#include <sstream>

using Microsoft::WRL::ComPtr;

namespace AccessOS {

static constexpr const char* kComponent = "SapiSpeechEngine";

SapiSpeechEngine::SapiSpeechEngine() = default;

SapiSpeechEngine::~SapiSpeechEngine() {
    Shutdown();
}

Result<void> SapiSpeechEngine::Initialize() {
    if (m_initialized) {
        ACOS_LOG_WARNING(kComponent, "Initialize() called when already initialized");
        return Result<void>::Ok();
    }

    ACOS_LOG_INFO(kComponent, "Initializing SAPI speech engine");

    // Create the SAPI ISpVoice instance.
    // CLSID_SpVoice is defined in sapi.h.
    HRESULT hr = ::CoCreateInstance(
        CLSID_SpVoice,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&m_voice));

    if (FAILED(hr) || !m_voice) {
        std::ostringstream oss;
        oss << "CoCreateInstance(ISpVoice) failed: HRESULT=0x" << std::hex << hr;
        ACOS_LOG_CRITICAL(kComponent, oss.str());
        return Result<void>::Fail(
            MakeError(ErrorCode::SpeechEngineUnavailable), oss.str());
    }

    // Set default rate and volume.
    m_voice->SetRate(0);
    m_voice->SetVolume(100);

    m_initialized = true;
    ACOS_LOG_INFO(kComponent, "SAPI speech engine initialized");
    return Result<void>::Ok();
}

void SapiSpeechEngine::Shutdown() {
    if (!m_initialized) return;
    ACOS_LOG_INFO(kComponent, "Shutting down SAPI speech engine");
    if (m_voice) {
        m_voice->Speak(nullptr, SPF_PURGEBEFORESPEAK, nullptr);
        m_voice.Reset();
    }
    m_initialized = false;
}

bool SapiSpeechEngine::IsAvailable() const noexcept {
    return m_initialized && m_voice != nullptr;
}

Result<void> SapiSpeechEngine::Speak(const std::string& text) {
    if (!IsAvailable()) {
        return Result<void>::Fail(
            MakeError(ErrorCode::SpeechEngineUnavailable),
            "SAPI engine not initialized");
    }

    if (text.empty()) return Result<void>::Ok();

    std::wstring wide = Utf8ToWide(text);

    // SPF_IS_XML allows SSML-like markup in future. SPF_DEFAULT for plain text now.
    // SVSFlagsAsync would be non-blocking, but we use synchronous mode on the
    // dedicated speech thread to simplify queue management.
    HRESULT hr = m_voice->Speak(wide.c_str(), SPF_DEFAULT, nullptr);

    if (FAILED(hr)) {
        std::ostringstream oss;
        oss << "ISpVoice::Speak failed: HRESULT=0x" << std::hex << hr;
        ACOS_LOG_ERROR(kComponent, oss.str());
        return Result<void>::Fail(MakeError(ErrorCode::SpeechEngineError), oss.str());
    }

    return Result<void>::Ok();
}

void SapiSpeechEngine::Stop() {
    if (!IsAvailable()) return;
    // SPF_PURGEBEFORESPEAK stops current utterance and clears SAPI internal queue.
    m_voice->Speak(nullptr, SPF_PURGEBEFORESPEAK, nullptr);
    m_paused = false;
}

void SapiSpeechEngine::Pause() {
    if (!IsAvailable() || m_paused) return;
    m_voice->Pause();
    m_paused = true;
}

void SapiSpeechEngine::Resume() {
    if (!IsAvailable() || !m_paused) return;
    m_voice->Resume();
    m_paused = false;
}

Result<void> SapiSpeechEngine::SetRate(int rate) {
    if (!IsAvailable()) {
        return Result<void>::Fail(
            MakeError(ErrorCode::SpeechEngineUnavailable), "Not initialized");
    }
    // SAPI rate: -10 to +10
    const long sapiRate = static_cast<long>(
        std::max(-10, std::min(10, rate)));
    HRESULT hr = m_voice->SetRate(sapiRate);
    if (FAILED(hr)) {
        return Result<void>::Fail(
            MakeError(ErrorCode::SpeechEngineError), "SetRate failed");
    }
    return Result<void>::Ok();
}

Result<void> SapiSpeechEngine::SetVolume(int volume) {
    if (!IsAvailable()) {
        return Result<void>::Fail(
            MakeError(ErrorCode::SpeechEngineUnavailable), "Not initialized");
    }
    // SAPI volume: 0–100
    const USHORT sapiVol = static_cast<USHORT>(
        std::max(0, std::min(100, volume)));
    HRESULT hr = m_voice->SetVolume(sapiVol);
    if (FAILED(hr)) {
        return Result<void>::Fail(
            MakeError(ErrorCode::SpeechEngineError), "SetVolume failed");
    }
    return Result<void>::Ok();
}

Result<void> SapiSpeechEngine::SetVoice(const std::string& voiceId) {
    if (!IsAvailable()) {
        return Result<void>::Fail(
            MakeError(ErrorCode::SpeechEngineUnavailable), "Not initialized");
    }

    // Enumerate voices and find a match by ID or name.
    // Direct COM enumeration — avoids sphelper.h (ATL dependency).
    ComPtr<ISpObjectTokenCategory> category;
    ComPtr<IEnumSpObjectTokens>    tokens;

    HRESULT hr = ::CoCreateInstance(
        CLSID_SpObjectTokenCategory,
        nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&category));
    if (FAILED(hr)) {
        return Result<void>::Fail(
            MakeError(ErrorCode::SpeechEngineError), "Create SpObjectTokenCategory failed");
    }
    hr = category->SetId(SPCAT_VOICES, FALSE);
    if (FAILED(hr)) {
        return Result<void>::Fail(
            MakeError(ErrorCode::SpeechEngineError), "SetId(SPCAT_VOICES) failed");
    }

    hr = category->EnumTokens(nullptr, nullptr, &tokens);
    if (FAILED(hr)) {
        return Result<void>::Fail(
            MakeError(ErrorCode::SpeechEngineError), "EnumTokens failed");
    }

    ComPtr<ISpObjectToken> token;
    while (tokens->Next(1, &token, nullptr) == S_OK) {
        WCHAR* idStr = nullptr;
        if (SUCCEEDED(token->GetId(&idStr)) && idStr) {
            std::wstring wid(idStr);
            ::CoTaskMemFree(idStr);
            // Match by ID suffix or full ID.
            std::wstring wvoiceId = Utf8ToWide(voiceId);
            if (wid.find(wvoiceId) != std::wstring::npos) {
                hr = m_voice->SetVoice(token.Get());
                if (FAILED(hr)) {
                    return Result<void>::Fail(
                        MakeError(ErrorCode::SpeechEngineError), "SetVoice failed");
                }
                return Result<void>::Ok();
            }
        }
        token.Reset();
    }

    return Result<void>::Fail(
        MakeError(ErrorCode::SpeechEngineError),
        "Voice not found: " + voiceId);
}

std::vector<VoiceInfo> SapiSpeechEngine::GetAvailableVoices() const {
    std::vector<VoiceInfo> voices;
    if (!IsAvailable()) return voices;

    // Direct COM enumeration — avoids sphelper.h (ATL dependency).
    ComPtr<ISpObjectTokenCategory> category;
    ComPtr<IEnumSpObjectTokens>    tokens;

    if (FAILED(::CoCreateInstance(CLSID_SpObjectTokenCategory, nullptr,
                                  CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&category))))
        return voices;
    if (FAILED(category->SetId(SPCAT_VOICES, FALSE))) return voices;
    if (FAILED(category->EnumTokens(nullptr, nullptr, &tokens))) return voices;

    // Helper: convert WCHAR* (CoTaskMem) to std::string UTF-8
    auto WideToUtf8 = [](WCHAR* w) -> std::string {
        if (!w) return {};
        int len = ::WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
        if (len <= 0) return {};
        std::string s(static_cast<size_t>(len - 1), '\0');
        ::WideCharToMultiByte(CP_UTF8, 0, w, -1, s.data(), len, nullptr, nullptr);
        return s;
    };

    ComPtr<ISpObjectToken> token;
    while (tokens->Next(1, &token, nullptr) == S_OK) {
        VoiceInfo info;

        WCHAR* idStr = nullptr;
        if (SUCCEEDED(token->GetId(&idStr)) && idStr) {
            info.id = WideToUtf8(idStr);
            ::CoTaskMemFree(idStr);
        }

        // Get voice description from the token's default attribute.
        // ISpDataKey::GetStringValue reads named attributes from the registry.
        WCHAR* nameStr = nullptr;
        if (SUCCEEDED(token->GetStringValue(L"", &nameStr)) && nameStr) {
            info.name = WideToUtf8(nameStr);
            ::CoTaskMemFree(nameStr);
        }

        info.language = "en"; // Language detection via attributes — deferred
        voices.push_back(std::move(info));
        token.Reset();
    }

    return voices;
}

const char* SapiSpeechEngine::EngineName() const noexcept {
    return "SAPI";
}

std::wstring SapiSpeechEngine::Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return {};
    int len = ::MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                                    static_cast<int>(utf8.size()),
                                    nullptr, 0);
    if (len <= 0) return {};
    std::wstring wide(static_cast<size_t>(len), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                          static_cast<int>(utf8.size()),
                          wide.data(), len);
    return wide;
}

} // namespace AccessOS
