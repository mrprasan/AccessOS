// Test_AudioSubsystem.cpp — ACCESSOS-024 Audio/Earcon subsystem tests
// Covers: EarconId names, ToneParams, EarconManager lifecycle, mute/enable, volume

#include <gtest/gtest.h>
#include <algorithm>
#include "../../src/Core/Audio/EarconId.h"
#include "../../src/Core/Audio/IAudioEngine.h"
#include "../../src/Core/Audio/EarconManager.h"

using namespace AccessOS::Audio;

// ── Stub audio engine ─────────────────────────────────────────────────────────

class StubAudioEngine : public IAudioEngine {
public:
    explicit StubAudioEngine(bool available = true) : m_available(available) {}

    bool  IsAvailable() const override { return m_available; }

    bool PlayTone(const ToneParams& params) override {
        if (m_muted) return true;  // muted: silent but success
        ++m_playCount;
        m_lastParams = params;
        return m_available;
    }

    void  SetVolume(float v) override { m_volume = std::clamp(v, 0.0f, 1.0f); }
    float GetVolume()  const override { return m_volume; }
    void  SetMuted(bool m) override { m_muted = m; }
    bool  IsMuted()    const override { return m_muted; }

    // Test accessors
    int PlayCount()            const { return m_playCount; }
    const ToneParams& LastParams() const { return m_lastParams; }

private:
    bool      m_available;
    int       m_playCount = 0;
    ToneParams m_lastParams{};
    float     m_volume    = 0.7f;
    bool      m_muted     = false;
};

// ── EarconId tests ────────────────────────────────────────────────────────────

TEST(EarconId, NoneIsZero) {
    EXPECT_EQ(static_cast<uint32_t>(EarconId::None), 0u);
}

TEST(EarconId, EarconNameNone) {
    EXPECT_EQ(EarconName(EarconId::None), "None");
}

TEST(EarconId, EarconNameKnown) {
    EXPECT_EQ(EarconName(EarconId::FocusEnter), "FocusEnter");
    EXPECT_EQ(EarconName(EarconId::Error),      "Error");
    EXPECT_EQ(EarconName(EarconId::Boundary),   "Boundary");
}

TEST(EarconId, EarconNameUnknown) {
    EXPECT_EQ(EarconName(static_cast<EarconId>(9999)), "Unknown");
}

TEST(EarconId, AllNamedIdsHaveNames) {
    // Spot-check all declared ids
    EarconId ids[] = {
        EarconId::FocusEnter, EarconId::FocusLeave,
        EarconId::Link, EarconId::Button, EarconId::Checkbox,
        EarconId::Checked, EarconId::Unchecked,
        EarconId::Alert, EarconId::Error, EarconId::Warning, EarconId::Success,
        EarconId::DocumentStart, EarconId::DocumentEnd, EarconId::Boundary,
    };
    for (auto id : ids) {
        EXPECT_NE(EarconName(id), "Unknown") << "EarconId " << static_cast<int>(id);
    }
}

// ── ToneParams tests ──────────────────────────────────────────────────────────

TEST(ToneParams, Defaults) {
    ToneParams p;
    EXPECT_FLOAT_EQ(p.frequencyHz, 440.0f);
    EXPECT_FLOAT_EQ(p.durationMs,  80.0f);
    EXPECT_FLOAT_EQ(p.volume,      0.7f);
    EXPECT_FLOAT_EQ(p.fadeInMs,    5.0f);
    EXPECT_FLOAT_EQ(p.fadeOutMs,   10.0f);
}

// ── EarconManager lifecycle ───────────────────────────────────────────────────

TEST(EarconManager, DefaultConstructorNotAvailable) {
    EarconManager mgr;
    EXPECT_FALSE(mgr.IsAvailable());
}

TEST(EarconManager, SetEngineAvailable) {
    EarconManager mgr;
    mgr.SetEngine(std::make_shared<StubAudioEngine>(true));
    EXPECT_TRUE(mgr.IsAvailable());
}

TEST(EarconManager, SetEngineUnavailable) {
    EarconManager mgr;
    mgr.SetEngine(std::make_shared<StubAudioEngine>(false));
    EXPECT_FALSE(mgr.IsAvailable());
}

TEST(EarconManager, GetEngineReturnsSet) {
    auto eng = std::make_shared<StubAudioEngine>();
    EarconManager mgr(eng);
    EXPECT_EQ(mgr.GetEngine(), eng);
}

TEST(EarconManager, PlayNoneReturnsFalse) {
    EarconManager mgr(std::make_shared<StubAudioEngine>());
    EXPECT_FALSE(mgr.Play(EarconId::None));
}

TEST(EarconManager, PlayKnownEarconCallsEngine) {
    auto stub = std::make_shared<StubAudioEngine>();
    EarconManager mgr(stub);
    EXPECT_TRUE(mgr.Play(EarconId::FocusEnter));
    EXPECT_EQ(stub->PlayCount(), 1);
}

TEST(EarconManager, PlayedToneHasNonZeroFrequency) {
    auto stub = std::make_shared<StubAudioEngine>();
    EarconManager mgr(stub);
    mgr.Play(EarconId::Button);
    EXPECT_GT(stub->LastParams().frequencyHz, 0.0f);
}

TEST(EarconManager, PlayedToneHasNonZeroDuration) {
    auto stub = std::make_shared<StubAudioEngine>();
    EarconManager mgr(stub);
    mgr.Play(EarconId::Alert);
    EXPECT_GT(stub->LastParams().durationMs, 0.0f);
}

// ── Tone table customisation ──────────────────────────────────────────────────

TEST(EarconManager, SetToneParamsApplied) {
    auto stub = std::make_shared<StubAudioEngine>();
    EarconManager mgr(stub);

    ToneParams custom;
    custom.frequencyHz = 1234.5f;
    custom.durationMs  = 99.0f;
    mgr.SetToneParams(EarconId::Link, custom);
    mgr.Play(EarconId::Link);

    EXPECT_FLOAT_EQ(stub->LastParams().frequencyHz, 1234.5f);
    EXPECT_FLOAT_EQ(stub->LastParams().durationMs,  99.0f);
}

TEST(EarconManager, GetToneParamsReturnsSet) {
    EarconManager mgr(std::make_shared<StubAudioEngine>());
    ToneParams p;
    p.frequencyHz = 500.0f;
    p.durationMs  = 50.0f;
    mgr.SetToneParams(EarconId::Error, p);
    ToneParams got = mgr.GetToneParams(EarconId::Error);
    EXPECT_FLOAT_EQ(got.frequencyHz, 500.0f);
}

TEST(EarconManager, DifferentEarconsDifferentFrequencies) {
    EarconManager mgr;
    ToneParams a = mgr.GetToneParams(EarconId::FocusEnter);
    ToneParams b = mgr.GetToneParams(EarconId::Error);
    // These should not be the same default frequency
    EXPECT_NE(a.frequencyHz, b.frequencyHz);
}

// ── Per-earcon enable/disable ─────────────────────────────────────────────────

TEST(EarconManager, EarconEnabledByDefault) {
    EarconManager mgr(std::make_shared<StubAudioEngine>());
    EXPECT_TRUE(mgr.IsEarconEnabled(EarconId::FocusEnter));
}

TEST(EarconManager, DisableEarconSuppressesPlay) {
    auto stub = std::make_shared<StubAudioEngine>();
    EarconManager mgr(stub);
    mgr.SetEarconEnabled(EarconId::FocusEnter, false);
    bool result = mgr.Play(EarconId::FocusEnter);
    EXPECT_FALSE(result);
    EXPECT_EQ(stub->PlayCount(), 0);
}

TEST(EarconManager, ReEnableEarconAllowsPlay) {
    auto stub = std::make_shared<StubAudioEngine>();
    EarconManager mgr(stub);
    mgr.SetEarconEnabled(EarconId::FocusEnter, false);
    mgr.SetEarconEnabled(EarconId::FocusEnter, true);
    EXPECT_TRUE(mgr.Play(EarconId::FocusEnter));
    EXPECT_EQ(stub->PlayCount(), 1);
}

TEST(EarconManager, OtherEarconsUnaffectedByDisable) {
    auto stub = std::make_shared<StubAudioEngine>();
    EarconManager mgr(stub);
    mgr.SetEarconEnabled(EarconId::FocusEnter, false);
    mgr.Play(EarconId::Button); // should still play
    EXPECT_EQ(stub->PlayCount(), 1);
}

// ── Master enable/disable ─────────────────────────────────────────────────────

TEST(EarconManager, MasterEnabledByDefault) {
    EarconManager mgr(std::make_shared<StubAudioEngine>());
    EXPECT_TRUE(mgr.IsEnabled());
}

TEST(EarconManager, MasterDisableSuppressesAllPlay) {
    auto stub = std::make_shared<StubAudioEngine>();
    EarconManager mgr(stub);
    mgr.SetEnabled(false);
    mgr.Play(EarconId::FocusEnter);
    mgr.Play(EarconId::Alert);
    EXPECT_EQ(stub->PlayCount(), 0);
}

TEST(EarconManager, MasterReEnableRestoresPlay) {
    auto stub = std::make_shared<StubAudioEngine>();
    EarconManager mgr(stub);
    mgr.SetEnabled(false);
    mgr.SetEnabled(true);
    mgr.Play(EarconId::FocusEnter);
    EXPECT_EQ(stub->PlayCount(), 1);
}

// ── Volume ────────────────────────────────────────────────────────────────────

TEST(EarconManager, SetVolumeForwardedToEngine) {
    auto stub = std::make_shared<StubAudioEngine>();
    EarconManager mgr(stub);
    mgr.SetVolume(0.5f);
    EXPECT_FLOAT_EQ(mgr.GetVolume(), 0.5f);
}

TEST(EarconManager, VolumeZeroEngine) {
    EarconManager mgr; // no engine
    mgr.SetVolume(0.5f);
    EXPECT_FLOAT_EQ(mgr.GetVolume(), 0.0f); // no engine → 0
}

// ── Mute ──────────────────────────────────────────────────────────────────────

TEST(EarconManager, MuteForwardedToEngine) {
    auto stub = std::make_shared<StubAudioEngine>();
    EarconManager mgr(stub);
    mgr.SetMuted(true);
    EXPECT_TRUE(mgr.IsMuted());
    EXPECT_TRUE(stub->IsMuted());
}

TEST(EarconManager, MutedPlayReturnsTrueButNoSound) {
    auto stub = std::make_shared<StubAudioEngine>();
    EarconManager mgr(stub);
    mgr.SetMuted(true);
    // PlayTone on a muted engine returns true (not an error)
    // but play count is 0 because stub short-circuits when muted
    bool result = mgr.Play(EarconId::FocusEnter);
    EXPECT_TRUE(result);
    EXPECT_EQ(stub->PlayCount(), 0); // muted → no actual playback counted
}

TEST(EarconManager, UnmuteRestoredPlay) {
    auto stub = std::make_shared<StubAudioEngine>();
    EarconManager mgr(stub);
    mgr.SetMuted(true);
    mgr.SetMuted(false);
    mgr.Play(EarconId::FocusEnter);
    EXPECT_EQ(stub->PlayCount(), 1);
}

TEST(EarconManager, NoEngineIsMutedReturnsFalse) {
    EarconManager mgr;
    EXPECT_FALSE(mgr.IsMuted());
}
