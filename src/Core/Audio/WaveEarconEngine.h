#pragma once
// WaveEarconEngine.h — Win32 waveOut sine-tone earcon engine (ACCESSOS-024)
//
// Generates PCM sine-wave buffers and plays them via the Win32 waveOut API.
// Each PlayTone() call is asynchronous: buffer is allocated, waveOutWrite is
// called with CALLBACK_NULL, and the buffer is freed in the next call or
// on destruction via waveOutReset.
//
// Status: IMPLEMENTED (sine synthesis + waveOut playback)

#pragma once
#include "IAudioEngine.h"
#include <windows.h>
#include <mmsystem.h>
#include <vector>
#include <mutex>
#include <atomic>

#pragma comment(lib, "winmm.lib")

namespace AccessOS {
namespace Audio {

class WaveEarconEngine : public IAudioEngine {
public:
    WaveEarconEngine();
    ~WaveEarconEngine() override;

    bool  IsAvailable() const override;
    bool  PlayTone(const ToneParams& params) override;
    void  SetVolume(float volume) override;
    float GetVolume() const override;
    void  SetMuted(bool muted) override;
    bool  IsMuted() const override;

private:
    static constexpr uint32_t kSampleRate = 44100;
    static constexpr uint16_t kChannels   = 1;
    static constexpr uint16_t kBitsPerSample = 16;

    HWAVEOUT   m_hWaveOut = nullptr;
    bool       m_available = false;
    std::atomic<float> m_volume{ 0.7f };
    std::atomic<bool>  m_muted{ false };
    mutable std::mutex m_mutex;

    // Active buffer that must be freed after playback completes
    struct PlayBuffer {
        WAVEHDR        header{};
        std::vector<int16_t> samples;
    };
    std::unique_ptr<PlayBuffer> m_pending;

    // Open waveOut device; called once on first PlayTone
    bool OpenDevice();
    void CloseDevice();

    // Synthesise a sine tone into a sample buffer with linear fade in/out
    static std::vector<int16_t> SynthesizeTone(const ToneParams& params,
                                                float masterVolume);
};

} // namespace Audio
} // namespace AccessOS
