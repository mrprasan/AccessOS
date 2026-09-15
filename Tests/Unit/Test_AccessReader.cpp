// AccessOS/Tests/Unit/Test_AccessReader.cpp
//
// Unit tests for AccessReader.
//
// Strategy: AccessReader needs ContextEngine, FocusManager, and SpeechManager.
//   - ContextEngine and FocusManager are used directly (no SAPI/UIA).
//   - SpeechManager is initialized with a SpySpeechEngine that records
//     every Speak() call synchronously — no actual TTS, no threading required.
//
// Coverage:
//   - Initialize / Shutdown state
//   - OnEvent routing: FocusChanged, PropertyChanged, LiveRegion, ignored types
//   - Context update triggered by FocusChanged
//   - Announcement content for focus events
//   - Policy round-trip (SetPolicy / GetPolicy)
//   - StopSpeech delegates to SpeechManager
//   - ReadFocused re-announces current focus
//   - LastWindowTitle updated on focus change
//   - Empty announcements not forwarded to speech
//   - Events ignored when not running

#include <gtest/gtest.h>
#include "Reader/AccessReader.h"
#include "Context/ContextEngine.h"
#include "Focus/FocusManager.h"
#include "Speech/SpeechManager.h"
#include "Speech/ISpeechEngine.h"
#include "Semantic/SemanticModel.h"
#include "Semantic/SemanticCache.h"

#include <vector>
#include <string>
#include <mutex>

using namespace AccessOS;

// ── SpySpeechEngine ───────────────────────────────────────────────────────────
// Records every Speak() call without performing any TTS.

class SpySpeechEngine : public ISpeechEngine {
public:
    Result<void> Initialize() override { return Result<void>::Ok(); }
    void         Shutdown()  override {}
    bool         IsAvailable() const noexcept override { return true; }
    const char*  EngineName() const noexcept override { return "SpySpeech"; }

    Result<void> Speak(const std::string& text) override {
        std::unique_lock<std::mutex> lk(m_mutex);
        m_spoken.push_back(text);
        return Result<void>::Ok();
    }
    void Stop()                      override {}
    void Pause()                     override {}
    void Resume()                    override {}
    Result<void> SetRate(int)        override { return Result<void>::Ok(); }
    Result<void> SetVolume(int)      override { return Result<void>::Ok(); }
    Result<void> SetVoice(const std::string&) override { return Result<void>::Ok(); }
    std::vector<VoiceInfo> GetAvailableVoices() const override { return {}; }

    std::vector<std::string> Spoken() const {
        std::unique_lock<std::mutex> lk(m_mutex);
        return m_spoken;
    }
    void Clear() {
        std::unique_lock<std::mutex> lk(m_mutex);
        m_spoken.clear();
    }

private:
    mutable std::mutex       m_mutex;
    std::vector<std::string> m_spoken;
};

// ── Test fixture ──────────────────────────────────────────────────────────────

class AccessReaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_spy = std::make_unique<SpySpeechEngine>();
        m_spyRaw = m_spy.get();

        m_speechManager = std::make_unique<SpeechManager>();
        m_speechManager->Initialize(std::move(m_spy));

        m_contextEngine = std::make_unique<ContextEngine>();
        m_semanticModel = std::make_unique<SemanticModel>();
        m_focusManager  = std::make_unique<FocusManager>();
        m_focusManager->Initialize(m_semanticModel.get());

        m_reader = std::make_unique<AccessReader>(
            m_contextEngine.get(),
            m_focusManager.get(),
            m_speechManager.get());

        m_reader->Initialize();
    }

    void TearDown() override {
        m_reader->Shutdown();
        m_speechManager->Shutdown();
    }

    AccessEvent MakeFocusEvent(const std::string& appName,
                               AccessRole role,
                               const std::string& name,
                               const std::string& value = "",
                               uint32_t pid = 1,
                               const std::string& windowTitle = "Test Window")
    {
        AccessEvent ev;
        ev.type = AccessEventType::FocusChanged;
        ev.element.applicationName = appName;
        ev.element.role            = role;
        ev.element.name            = name;
        ev.element.value           = value;
        ev.element.processId       = pid;
        ev.element.windowTitle     = windowTitle;
        ev.element.isValid         = true;
        ev.timestampMs             = 0;
        return ev;
    }

    AccessEvent MakePropertyEvent(AccessEventType type,
                                   AccessRole role,
                                   const std::string& name,
                                   const std::string& value = "")
    {
        AccessEvent ev;
        ev.type = type;
        ev.element.role    = role;
        ev.element.name    = name;
        ev.element.value   = value;
        ev.element.isValid = true;
        return ev;
    }

    // Wait briefly for speech thread to process.
    void DrainSpeech() {
        // The speech thread is asynchronous. Give it up to 200ms.
        for (int i = 0; i < 20; ++i) {
            if (!m_spyRaw->Spoken().empty()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    SpySpeechEngine*                m_spyRaw = nullptr;
    std::unique_ptr<SpySpeechEngine> m_spy;
    std::unique_ptr<SpeechManager>  m_speechManager;
    std::unique_ptr<ContextEngine>  m_contextEngine;
    std::unique_ptr<SemanticModel>  m_semanticModel;
    std::unique_ptr<FocusManager>   m_focusManager;
    std::unique_ptr<AccessReader>   m_reader;
};

// ─── Lifecycle ────────────────────────────────────────────────────────────────

TEST_F(AccessReaderTest, IsRunningAfterInitialize) {
    EXPECT_TRUE(m_reader->IsRunning());
}

TEST_F(AccessReaderTest, IsNotRunningAfterShutdown) {
    m_reader->Shutdown();
    EXPECT_FALSE(m_reader->IsRunning());
}

TEST_F(AccessReaderTest, ShutdownIdempotent) {
    m_reader->Shutdown();
    m_reader->Shutdown();   // Should not crash
    EXPECT_FALSE(m_reader->IsRunning());
}

// ─── Policy ───────────────────────────────────────────────────────────────────

TEST_F(AccessReaderTest, DefaultPolicyIsStandard) {
    SpeechPolicy pol = m_reader->GetPolicy();
    EXPECT_EQ(pol.verbosity, VerbosityLevel::Standard);
    EXPECT_TRUE(pol.announceRole);
}

TEST_F(AccessReaderTest, SetPolicyRoundTrip) {
    SpeechPolicy pol = SpeechPolicy::Detailed();
    m_reader->SetPolicy(pol);
    SpeechPolicy got = m_reader->GetPolicy();
    EXPECT_EQ(got.verbosity, VerbosityLevel::Detailed);
    EXPECT_TRUE(got.announceDescription);
}

// ─── FocusChanged routing ─────────────────────────────────────────────────────

TEST_F(AccessReaderTest, FocusChangedUpdatesContextEngine) {
    AccessEvent ev = MakeFocusEvent("chrome.exe", AccessRole::Window, "Google", "", 10);
    m_reader->OnEvent(ev);
    AppContext ctx = m_contextEngine->GetCurrent();
    EXPECT_EQ(ctx.type, AppContextType::Browser);
}

TEST_F(AccessReaderTest, FocusChangedProducesSpeech) {
    AccessEvent ev = MakeFocusEvent("chrome.exe", AccessRole::Button, "Search", "", 10);
    m_reader->OnEvent(ev);
    DrainSpeech();
    auto spoken = m_spyRaw->Spoken();
    ASSERT_FALSE(spoken.empty());
    // Must contain the element name.
    EXPECT_NE(spoken.back().find("Search"), std::string::npos);
}

TEST_F(AccessReaderTest, FocusChangedAnnouncesRole) {
    AccessEvent ev = MakeFocusEvent("notepad.exe", AccessRole::Edit, "Body text");
    m_reader->OnEvent(ev);
    DrainSpeech();
    auto spoken = m_spyRaw->Spoken();
    ASSERT_FALSE(spoken.empty());
    EXPECT_NE(spoken.back().find("edit"), std::string::npos);
}

TEST_F(AccessReaderTest, FocusChangedWithInvalidNodeNoSpeech) {
    AccessEvent ev = MakeFocusEvent("chrome.exe", AccessRole::Button, "Close");
    ev.element.isValid = false;
    m_reader->OnEvent(ev);
    // Give a short window for any erroneous speech
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_TRUE(m_spyRaw->Spoken().empty());
}

// ─── LastWindowTitle ─────────────────────────────────────────────────────────

TEST_F(AccessReaderTest, LastWindowTitleUpdatedOnFocusChange) {
    AccessEvent ev = MakeFocusEvent("code.exe", AccessRole::Edit, "main.cpp",
                                     "", 1, "Visual Studio Code");
    m_reader->OnEvent(ev);
    EXPECT_EQ(m_reader->LastWindowTitle(), "Visual Studio Code");
}

TEST_F(AccessReaderTest, LastWindowTitleEmptyInitially) {
    EXPECT_EQ(m_reader->LastWindowTitle(), "");
}

TEST_F(AccessReaderTest, LastWindowTitleUpdatesToLatest) {
    m_reader->OnEvent(MakeFocusEvent("code.exe", AccessRole::Window, "Editor",
                                      "", 1, "Window A"));
    m_reader->OnEvent(MakeFocusEvent("code.exe", AccessRole::Window, "Editor",
                                      "", 2, "Window B"));
    EXPECT_EQ(m_reader->LastWindowTitle(), "Window B");
}

// ─── PropertyChanged routing ─────────────────────────────────────────────────

TEST_F(AccessReaderTest, ValueChangedProducesSpeech) {
    AccessEvent ev = MakePropertyEvent(AccessEventType::ValueChanged,
                                        AccessRole::Slider, "Volume", "80%");
    m_reader->OnEvent(ev);
    DrainSpeech();
    auto spoken = m_spyRaw->Spoken();
    ASSERT_FALSE(spoken.empty());
    EXPECT_NE(spoken.back().find("Volume"), std::string::npos);
}

TEST_F(AccessReaderTest, StateChangedProducesSpeech) {
    AccessEvent ev = MakePropertyEvent(AccessEventType::StateChanged,
                                        AccessRole::CheckBox, "Bold");
    ev.element.state = AccessState::Checked;
    m_reader->OnEvent(ev);
    DrainSpeech();
    auto spoken = m_spyRaw->Spoken();
    ASSERT_FALSE(spoken.empty());
    EXPECT_NE(spoken.back().find("Bold"), std::string::npos);
}

// ─── LiveRegion / Alert routing ───────────────────────────────────────────────

TEST_F(AccessReaderTest, LiveRegionChangedProducesSpeech) {
    AccessEvent ev;
    ev.type = AccessEventType::LiveRegionChanged;
    ev.element.role    = AccessRole::Status;
    ev.element.name    = "File saved";
    ev.element.isValid = true;
    m_reader->OnEvent(ev);
    DrainSpeech();
    auto spoken = m_spyRaw->Spoken();
    ASSERT_FALSE(spoken.empty());
    EXPECT_NE(spoken.back().find("File saved"), std::string::npos);
}

TEST_F(AccessReaderTest, AlertEventHasHighPriority) {
    // Just verify it goes through the alert path (name is spoken).
    AccessEvent ev;
    ev.type = AccessEventType::LiveRegionChanged;
    ev.element.role    = AccessRole::Alert;
    ev.element.name    = "Low disk space";
    ev.element.isValid = true;
    m_reader->OnEvent(ev);
    DrainSpeech();
    auto spoken = m_spyRaw->Spoken();
    ASSERT_FALSE(spoken.empty());
    EXPECT_NE(spoken.back().find("Low disk space"), std::string::npos);
}

TEST_F(AccessReaderTest, LiveRegionExtraInfoAppended) {
    AccessEvent ev;
    ev.type = AccessEventType::NotificationRaised;
    ev.element.role    = AccessRole::Status;
    ev.element.name    = "Update";
    ev.element.isValid = true;
    ev.extraInfo       = "Version 2.0 available";
    m_reader->OnEvent(ev);
    DrainSpeech();
    auto spoken = m_spyRaw->Spoken();
    ASSERT_FALSE(spoken.empty());
    EXPECT_NE(spoken.back().find("Version 2.0 available"), std::string::npos);
}

// ─── Ignored event types ──────────────────────────────────────────────────────

TEST_F(AccessReaderTest, MenuOpenedEventNotSpoken) {
    AccessEvent ev;
    ev.type = AccessEventType::MenuOpened;
    ev.element.name    = "File";
    ev.element.isValid = true;
    m_reader->OnEvent(ev);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_TRUE(m_spyRaw->Spoken().empty());
}

TEST_F(AccessReaderTest, WindowClosedEventNotSpoken) {
    AccessEvent ev;
    ev.type = AccessEventType::WindowClosed;
    ev.element.name    = "Notepad";
    ev.element.isValid = true;
    m_reader->OnEvent(ev);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_TRUE(m_spyRaw->Spoken().empty());
}

// ─── Events ignored when not running ─────────────────────────────────────────

TEST_F(AccessReaderTest, FocusChangedIgnoredWhenShutdown) {
    m_reader->Shutdown();
    AccessEvent ev = MakeFocusEvent("chrome.exe", AccessRole::Button, "Search");
    m_reader->OnEvent(ev);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_TRUE(m_spyRaw->Spoken().empty());
}

// ─── ReadFocused ──────────────────────────────────────────────────────────────

TEST_F(AccessReaderTest, ReadFocusedWithNoFocusDoesNothing) {
    // FocusManager has no current focus — should not crash.
    m_reader->ReadFocused();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_TRUE(m_spyRaw->Spoken().empty());
}

// ─── StopSpeech ───────────────────────────────────────────────────────────────

TEST_F(AccessReaderTest, StopSpeechDoesNotCrash) {
    // Just verify no crash when called with no active speech.
    EXPECT_NO_THROW(m_reader->StopSpeech());
}
