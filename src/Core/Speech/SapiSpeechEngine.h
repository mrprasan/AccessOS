// AccessOS/src/Core/Speech/SapiSpeechEngine.h
//
// SAPI (Speech API) implementation of ISpeechEngine.
//
// Why: Windows Speech API is the initial TTS backend.
//      It is behind the ISpeechEngine interface so that
//      additional engines can be added without changing consumers.
//
// Threading: All methods must be called from a single thread
//            that has been CoInitialized (STA or MTA).
//            SpeechManager ensures this via a dedicated speech thread.

#pragma once

#include "ISpeechEngine.h"

// SAPI headers require COM and windows.h in the correct order.
// sphelper.h requires ATL (atlbase.h) which is not available in
// Visual Studio Build Tools without the ATL component. We enumerate
// voices directly via ISpObjectTokenCategory without sphelper.h.
#include <windows.h>
#include <sapi.h>
#include <wrl/client.h>

namespace AccessOS {

class SapiSpeechEngine final : public ISpeechEngine {
public:
    SapiSpeechEngine();
    ~SapiSpeechEngine() override;

    Result<void>            Initialize()                        override;
    void                    Shutdown()                          override;
    bool                    IsAvailable()     const noexcept    override;
    Result<void>            Speak(const std::string& text)      override;
    void                    Stop()                              override;
    void                    Pause()                             override;
    void                    Resume()                            override;
    Result<void>            SetRate(int rate)                   override;
    Result<void>            SetVolume(int volume)               override;
    Result<void>            SetVoice(const std::string& voiceId) override;
    std::vector<VoiceInfo>  GetAvailableVoices() const          override;
    const char*             EngineName() const noexcept         override;

private:
    // Converts std::string (UTF-8) to std::wstring for SAPI calls.
    static std::wstring Utf8ToWide(const std::string& utf8);

    Microsoft::WRL::ComPtr<ISpVoice>  m_voice;
    bool                              m_initialized{ false };
    bool                              m_paused{ false };
};

} // namespace AccessOS
