// WaveEarconEngine.cpp — Win32 waveOut sine-tone earcon engine (ACCESSOS-024)
#include "WaveEarconEngine.h"
#include <cmath>
#include <algorithm>
#include <numbers>

namespace AccessOS {
namespace Audio {

static constexpr float kPi = 3.14159265358979323846f;

WaveEarconEngine::WaveEarconEngine() {
    // Probe availability: try to open the default waveOut device
    WAVEFORMATEX wfx{};
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.nChannels       = kChannels;
    wfx.nSamplesPerSec  = kSampleRate;
    wfx.wBitsPerSample  = kBitsPerSample;
    wfx.nBlockAlign     = (kChannels * kBitsPerSample) / 8;
    wfx.nAvgBytesPerSec = kSampleRate * wfx.nBlockAlign;

    HWAVEOUT probe = nullptr;
    MMRESULT r = waveOutOpen(&probe, WAVE_MAPPER, &wfx, 0, 0, WAVE_FORMAT_QUERY);
    m_available = (r == MMSYSERR_NOERROR);
    if (probe) waveOutClose(probe);
}

WaveEarconEngine::~WaveEarconEngine() {
    CloseDevice();
}

bool WaveEarconEngine::IsAvailable() const {
    return m_available;
}

bool WaveEarconEngine::PlayTone(const ToneParams& params) {
    if (m_muted.load()) return true;  // muted: success but silent
    if (!m_available)   return false;

    float vol = std::clamp(m_volume.load(), 0.0f, 1.0f);
    auto samples = SynthesizeTone(params, vol);
    if (samples.empty()) return false;

    std::lock_guard<std::mutex> lk(m_mutex);

    // Free previous buffer if waveOut is done with it
    if (m_pending) {
        waveOutUnprepareHeader(m_hWaveOut, &m_pending->header, sizeof(WAVEHDR));
        m_pending.reset();
    }

    if (!OpenDevice()) return false;

    auto buf = std::make_unique<PlayBuffer>();
    buf->samples = std::move(samples);

    WAVEHDR& hdr = buf->header;
    hdr.lpData         = reinterpret_cast<LPSTR>(buf->samples.data());
    hdr.dwBufferLength = static_cast<DWORD>(buf->samples.size() * sizeof(int16_t));
    hdr.dwFlags        = 0;

    if (waveOutPrepareHeader(m_hWaveOut, &hdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
        return false;

    MMRESULT wr = waveOutWrite(m_hWaveOut, &hdr, sizeof(WAVEHDR));
    if (wr != MMSYSERR_NOERROR) {
        waveOutUnprepareHeader(m_hWaveOut, &hdr, sizeof(WAVEHDR));
        return false;
    }

    m_pending = std::move(buf);
    return true;
}

void WaveEarconEngine::SetVolume(float volume) {
    m_volume.store(std::clamp(volume, 0.0f, 1.0f));
}

float WaveEarconEngine::GetVolume() const {
    return m_volume.load();
}

void WaveEarconEngine::SetMuted(bool muted) {
    m_muted.store(muted);
}

bool WaveEarconEngine::IsMuted() const {
    return m_muted.load();
}

// ── private ───────────────────────────────────────────────────────────────────

bool WaveEarconEngine::OpenDevice() {
    if (m_hWaveOut) return true;

    WAVEFORMATEX wfx{};
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.nChannels       = kChannels;
    wfx.nSamplesPerSec  = kSampleRate;
    wfx.wBitsPerSample  = kBitsPerSample;
    wfx.nBlockAlign     = (kChannels * kBitsPerSample) / 8;
    wfx.nAvgBytesPerSec = kSampleRate * wfx.nBlockAlign;

    MMRESULT r = waveOutOpen(&m_hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    return r == MMSYSERR_NOERROR;
}

void WaveEarconEngine::CloseDevice() {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (!m_hWaveOut) return;
    waveOutReset(m_hWaveOut);
    if (m_pending) {
        waveOutUnprepareHeader(m_hWaveOut, &m_pending->header, sizeof(WAVEHDR));
        m_pending.reset();
    }
    waveOutClose(m_hWaveOut);
    m_hWaveOut = nullptr;
}

/*static*/
std::vector<int16_t> WaveEarconEngine::SynthesizeTone(const ToneParams& params,
                                                        float masterVolume) {
    if (params.durationMs <= 0.0f || params.frequencyHz <= 0.0f) return {};

    uint32_t totalSamples = static_cast<uint32_t>(
        (params.durationMs / 1000.0f) * static_cast<float>(kSampleRate));
    if (totalSamples == 0) return {};

    uint32_t fadeInSamples  = static_cast<uint32_t>(
        (params.fadeInMs  / 1000.0f) * kSampleRate);
    uint32_t fadeOutSamples = static_cast<uint32_t>(
        (params.fadeOutMs / 1000.0f) * kSampleRate);

    // Clamp fades so they don't exceed total length
    fadeInSamples  = std::min(fadeInSamples,  totalSamples / 2);
    fadeOutSamples = std::min(fadeOutSamples, totalSamples / 2);

    std::vector<int16_t> buf(totalSamples);
    float amp = std::clamp(params.volume, 0.0f, 1.0f) * masterVolume * 32767.0f;
    float angularFreq = 2.0f * kPi * params.frequencyHz / static_cast<float>(kSampleRate);

    for (uint32_t i = 0; i < totalSamples; ++i) {
        float envelope = 1.0f;
        if (i < fadeInSamples && fadeInSamples > 0) {
            envelope = static_cast<float>(i) / static_cast<float>(fadeInSamples);
        } else if (i >= totalSamples - fadeOutSamples && fadeOutSamples > 0) {
            uint32_t fromEnd = totalSamples - 1 - i;
            envelope = static_cast<float>(fromEnd) / static_cast<float>(fadeOutSamples);
        }
        float sample = std::sin(angularFreq * static_cast<float>(i)) * amp * envelope;
        buf[i] = static_cast<int16_t>(std::clamp(sample, -32768.0f, 32767.0f));
    }
    return buf;
}

} // namespace Audio
} // namespace AccessOS
