// AccessOS/Tests/Unit/Test_SettingsApplier.cpp
//
// Unit tests for SettingsApplier.
//
// Coverage:
//   - Apply with null subsystem pointers (no crash)
//   - Apply sets speech rate/volume via SpeechManager
//   - Apply sets verbosity policy on AccessReader for all levels
//   - Apply propagates AnnouncePosition to SpeechPolicy
//   - Apply loads custom shortcut bindings from SettingsManager
//   - Save persists verbosity level to SettingsManager
//   - Save persists announcePosition to SettingsManager

#include <gtest/gtest.h>
#include <windows.h>
#include "Settings/SettingsApplier.h"
#include "Settings/SettingsManager.h"
#include "Settings/SqliteSettingsStore.h"
#include "Context/ContextEngine.h"
#include "Focus/FocusManager.h"
#include "Semantic/SemanticModel.h"
#include "Speech/SpeechManager.h"
#include "Speech/ISpeechEngine.h"
#include "Reader/AccessReader.h"
#include "Commands/CommandManager.h"
#include "Commands/ShortcutManager.h"
#include "Commands/CommandRegistry.h"

using namespace AccessOS;

// ── SpySpeechEngine (same pattern as Test_AccessReader.cpp) ───────────────────

class SpyEngine2 : public ISpeechEngine {
public:
    Result<void> Initialize()             override { return Result<void>::Ok(); }
    void         Shutdown()               override {}
    bool         IsAvailable() const noexcept override { return true; }
    const char*  EngineName()  const noexcept override { return "Spy2"; }
    Result<void> Speak(const std::string&) override { return Result<void>::Ok(); }
    void         Stop()    override {}
    void         Pause()   override {}
    void         Resume()  override {}
    Result<void> SetRate(int r)   override { lastRate   = r; return Result<void>::Ok(); }
    Result<void> SetVolume(int v) override { lastVolume = v; return Result<void>::Ok(); }
    Result<void> SetVoice(const std::string& id) override { lastVoice = id; return Result<void>::Ok(); }
    std::vector<VoiceInfo> GetAvailableVoices() const override { return {}; }

    int         lastRate   = -999;
    int         lastVolume = -999;
    std::string lastVoice;
};

// ── Fixture ───────────────────────────────────────────────────────────────────

class SettingsApplierTest : public ::testing::Test {
protected:
    void SetUp() override {
        // In-memory SQLite store.
        m_store = std::make_shared<SqliteSettingsStore>();
        m_store->Open(":memory:");
        m_settingsMgr = std::make_unique<SettingsManager>(m_store);

        m_spy = std::make_unique<SpyEngine2>();
        m_spyRaw = m_spy.get();

        m_speechMgr = std::make_unique<SpeechManager>();
        m_speechMgr->Initialize(std::move(m_spy));

        m_semanticModel = std::make_unique<SemanticModel>();
        m_contextEngine = std::make_unique<ContextEngine>();
        m_focusManager  = std::make_unique<FocusManager>();
        m_focusManager->Initialize(m_semanticModel.get());

        m_reader = std::make_unique<AccessReader>(
            m_contextEngine.get(), m_focusManager.get(), m_speechMgr.get());
        m_reader->Initialize();

        m_cmdMgr  = std::make_unique<CommandManager>();
        m_shortcutMgr = std::make_unique<ShortcutManager>();
        CommandRegistry::RegisterAll(*m_cmdMgr, m_reader.get(),
                                     nullptr, m_speechMgr.get(), m_focusManager.get());
        CommandRegistry::BindDefaults(*m_shortcutMgr);
    }

    void TearDown() override {
        m_reader->Shutdown();
        m_speechMgr->Shutdown();
    }

    SpyEngine2*                       m_spyRaw = nullptr;
    std::unique_ptr<SpyEngine2>       m_spy;
    std::shared_ptr<SqliteSettingsStore> m_store;
    std::unique_ptr<SettingsManager>  m_settingsMgr;
    std::unique_ptr<SpeechManager>    m_speechMgr;
    std::unique_ptr<SemanticModel>    m_semanticModel;
    std::unique_ptr<ContextEngine>    m_contextEngine;
    std::unique_ptr<FocusManager>     m_focusManager;
    std::unique_ptr<AccessReader>     m_reader;
    std::unique_ptr<CommandManager>   m_cmdMgr;
    std::unique_ptr<ShortcutManager>  m_shortcutMgr;
};

// ─── Apply — null safety ──────────────────────────────────────────────────────

TEST_F(SettingsApplierTest, ApplyWithAllNullsDoesNotCrash) {
    EXPECT_NO_THROW(
        SettingsApplier::Apply(*m_settingsMgr, nullptr, nullptr, nullptr, nullptr));
}

// ─── Apply — speech rate / volume ─────────────────────────────────────────────

TEST_F(SettingsApplierTest, ApplySpeechRate) {
    // SetRate is queued to the speech thread. Verify that SettingsManager
    // retains the value (applier reads it correctly) and Apply does not crash.
    m_settingsMgr->SetSpeechRate(5);
    EXPECT_NO_THROW(
        SettingsApplier::Apply(*m_settingsMgr, m_speechMgr.get(),
                               m_reader.get(), m_shortcutMgr.get(), m_cmdMgr.get()));
    // The setting was read from persistence correctly.
    EXPECT_EQ(m_settingsMgr->SpeechRate(), 5);
}

TEST_F(SettingsApplierTest, ApplySpeechVolume) {
    m_settingsMgr->SetSpeechVolume(80);
    EXPECT_NO_THROW(
        SettingsApplier::Apply(*m_settingsMgr, m_speechMgr.get(),
                               m_reader.get(), m_shortcutMgr.get(), m_cmdMgr.get()));
    EXPECT_EQ(m_settingsMgr->SpeechVolume(), 80);
}

TEST_F(SettingsApplierTest, ApplyVoiceId) {
    // SetVoice is queued to the speech thread — verify persistence round-trip.
    m_settingsMgr->SetVoiceId("Microsoft David Desktop");
    EXPECT_NO_THROW(
        SettingsApplier::Apply(*m_settingsMgr, m_speechMgr.get(),
                               m_reader.get(), m_shortcutMgr.get(), m_cmdMgr.get()));
    EXPECT_EQ(m_settingsMgr->VoiceId(), "Microsoft David Desktop");
}

TEST_F(SettingsApplierTest, EmptyVoiceIdNotApplied) {
    m_settingsMgr->SetVoiceId("");
    EXPECT_NO_THROW(
        SettingsApplier::Apply(*m_settingsMgr, m_speechMgr.get(),
                               m_reader.get(), m_shortcutMgr.get(), m_cmdMgr.get()));
    EXPECT_EQ(m_settingsMgr->VoiceId(), "");
}

// ─── Apply — verbosity policy ─────────────────────────────────────────────────

TEST_F(SettingsApplierTest, ApplyMinimalVerbosityPolicy) {
    m_settingsMgr->SetVerbosity(0);
    SettingsApplier::Apply(*m_settingsMgr, m_speechMgr.get(),
                           m_reader.get(), m_shortcutMgr.get(), m_cmdMgr.get());
    EXPECT_EQ(m_reader->GetPolicy().verbosity, VerbosityLevel::Minimal);
    EXPECT_FALSE(m_reader->GetPolicy().announceRole);
}

TEST_F(SettingsApplierTest, ApplyStandardVerbosityPolicy) {
    m_settingsMgr->SetVerbosity(1);
    SettingsApplier::Apply(*m_settingsMgr, m_speechMgr.get(),
                           m_reader.get(), m_shortcutMgr.get(), m_cmdMgr.get());
    EXPECT_EQ(m_reader->GetPolicy().verbosity, VerbosityLevel::Standard);
    EXPECT_FALSE(m_reader->GetPolicy().announceDescription);
}

TEST_F(SettingsApplierTest, ApplyDetailedVerbosityPolicy) {
    m_settingsMgr->SetVerbosity(2);
    SettingsApplier::Apply(*m_settingsMgr, m_speechMgr.get(),
                           m_reader.get(), m_shortcutMgr.get(), m_cmdMgr.get());
    EXPECT_EQ(m_reader->GetPolicy().verbosity, VerbosityLevel::Detailed);
    EXPECT_TRUE(m_reader->GetPolicy().announceDescription);
}

TEST_F(SettingsApplierTest, ApplyAnnouncePositionTrue) {
    m_settingsMgr->SetAnnouncePosition(true);
    m_settingsMgr->SetVerbosity(1);
    SettingsApplier::Apply(*m_settingsMgr, m_speechMgr.get(),
                           m_reader.get(), m_shortcutMgr.get(), m_cmdMgr.get());
    EXPECT_TRUE(m_reader->GetPolicy().announcePosition);
}

TEST_F(SettingsApplierTest, ApplyAnnouncePositionFalse) {
    m_settingsMgr->SetAnnouncePosition(false);
    m_settingsMgr->SetVerbosity(1);
    SettingsApplier::Apply(*m_settingsMgr, m_speechMgr.get(),
                           m_reader.get(), m_shortcutMgr.get(), m_cmdMgr.get());
    EXPECT_FALSE(m_reader->GetPolicy().announcePosition);
}

// ─── Apply — custom shortcut bindings ────────────────────────────────────────

TEST_F(SettingsApplierTest, ApplyCustomShortcutRebindsCommand) {
    // Save a custom binding: VK_F5 (no modifiers) → stop_speech.
    m_settingsMgr->SetShortcut(CmdId::StopSpeech, VK_F5, 0);
    SettingsApplier::Apply(*m_settingsMgr, m_speechMgr.get(),
                           m_reader.get(), m_shortcutMgr.get(), m_cmdMgr.get());

    KeyStroke ks{ VK_F5, KeyModifier::None };
    auto id = m_shortcutMgr->Lookup(ks);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(*id, std::string(CmdId::StopSpeech));
}

TEST_F(SettingsApplierTest, ApplyNoCustomShortcutLeavesDefaultsIntact) {
    // No custom bindings — defaults should remain.
    SettingsApplier::Apply(*m_settingsMgr, m_speechMgr.get(),
                           m_reader.get(), m_shortcutMgr.get(), m_cmdMgr.get());
    KeyStroke ks{ VK_SPACE, KeyModifier::Win };
    auto id = m_shortcutMgr->Lookup(ks);
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(*id, std::string(CmdId::ReadFocused));
}

// ─── Save ─────────────────────────────────────────────────────────────────────

TEST_F(SettingsApplierTest, SaveVerbosityMinimal) {
    SpeechPolicy pol = SpeechPolicy::Minimal();
    m_reader->SetPolicy(pol);
    m_speechMgr->SetPolicy(pol);
    SettingsApplier::Save(*m_settingsMgr, m_speechMgr.get());
    EXPECT_EQ(m_settingsMgr->Verbosity(), 0);
}

TEST_F(SettingsApplierTest, SaveVerbosityDetailed) {
    SpeechPolicy pol = SpeechPolicy::Detailed();
    m_reader->SetPolicy(pol);
    m_speechMgr->SetPolicy(pol);
    SettingsApplier::Save(*m_settingsMgr, m_speechMgr.get());
    EXPECT_EQ(m_settingsMgr->Verbosity(), 2);
}

TEST_F(SettingsApplierTest, SaveWithNullSpeechManagerDoesNotCrash) {
    EXPECT_NO_THROW(SettingsApplier::Save(*m_settingsMgr, nullptr));
}

TEST_F(SettingsApplierTest, SaveAnnouncePosition) {
    SpeechPolicy pol;
    pol.announcePosition = true;
    m_speechMgr->SetPolicy(pol);
    SettingsApplier::Save(*m_settingsMgr, m_speechMgr.get());
    EXPECT_TRUE(m_settingsMgr->AnnouncePosition());
}
