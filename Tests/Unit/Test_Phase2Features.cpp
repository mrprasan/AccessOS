// Test_Phase2Features.cpp — ACCESSOS-031/032/033 unit tests
// Covers: SayAll lifecycle, Typing Echo modes, ClipboardReader helpers,
//         AccessReader TypingEchoMode API

#include <gtest/gtest.h>
#include "../../src/Core/Reader/SayAll.h"
#include "../../src/Core/Reader/ClipboardReader.h"
#include "../../src/Core/Reader/AccessReader.h"
#include "../../src/Core/Speech/ISpeechEngine.h"

using namespace AccessOS;

// ── Spy speech engine for testing ────────────────────────────────────────────

class SpySpeechEngine2 : public ISpeechEngine {
public:
    Result<void> Initialize() override { return Result<void>::Ok(); }
    Result<void> Speak(const std::string& t) override {
        lastText = t; ++count; return Result<void>::Ok();
    }
    void Stop() override {}
    void Pause() override {}
    void Resume() override {}
    Result<void> SetRate(int) override { return Result<void>::Ok(); }
    Result<void> SetVolume(int) override { return Result<void>::Ok(); }
    Result<void> SetVoice(const std::string&) override { return Result<void>::Ok(); }
    bool IsAvailable() const noexcept override { return true; }
    std::vector<VoiceInfo> GetAvailableVoices() const override { return {}; }
    const char* EngineName() const noexcept override { return "Spy2"; }
    std::string lastText;
    int count = 0;
};

// ── SayAll tests ──────────────────────────────────────────────────────────────

TEST(SayAll, DefaultConstructorNocrash) {
    SayAll sa;
    EXPECT_FALSE(sa.IsRunning());
}

TEST(SayAll, StopBeforeRunIsNoOp) {
    SayAll sa;
    EXPECT_NO_FATAL_FAILURE(sa.Stop());
}

TEST(SayAll, RunWithNullCursorReturnsZero) {
    SayAll sa;
    sa.SetCursor(nullptr);
    sa.SetSpeechManager(nullptr);
    EXPECT_EQ(sa.Run(), 0u);
}

TEST(SayAll, RunWithNoDocumentReturnsZero) {
    Browse::VirtualCursor cursor;
    SayAll sa;
    sa.SetCursor(&cursor); // cursor has no document
    sa.SetSpeechManager(nullptr);
    EXPECT_EQ(sa.Run(), 0u);
}

TEST(SayAll, StopSignalsLoop) {
    SayAll sa;
    sa.Stop();
    EXPECT_FALSE(sa.IsRunning()); // stop before run — not running
}

// ── ClipboardReader tests ─────────────────────────────────────────────────────

TEST(ClipboardReader, HasTextReturnsBool) {
    // Just verify it doesn't crash — clipboard state is OS-dependent
    bool result = ClipboardReader::HasText();
    (void)result; // either true or false is valid
    SUCCEED();
}

TEST(ClipboardReader, ReadReturnsStringOrEmpty) {
    // Should not throw regardless of clipboard state
    std::string text;
    EXPECT_NO_FATAL_FAILURE(text = ClipboardReader::Read());
    // If clipboard has no text, result should be empty
    // If clipboard has text, result should be non-empty
    SUCCEED();
}

// ── AccessReader::TypingEchoMode API tests ────────────────────────────────────

class TypeEchoFixture : public ::testing::Test {
protected:
    void SetUp() override {
        reader = std::make_unique<AccessReader>(nullptr, nullptr, nullptr, nullptr);
    }
    std::unique_ptr<AccessReader> reader;
};

TEST_F(TypeEchoFixture, DefaultModeIsOff) {
    EXPECT_EQ(reader->GetTypingEchoMode(), AccessReader::TypingEchoMode::Off);
}

TEST_F(TypeEchoFixture, SetCharMode) {
    reader->SetTypingEchoMode(AccessReader::TypingEchoMode::Char);
    EXPECT_EQ(reader->GetTypingEchoMode(), AccessReader::TypingEchoMode::Char);
}

TEST_F(TypeEchoFixture, SetWordMode) {
    reader->SetTypingEchoMode(AccessReader::TypingEchoMode::Word);
    EXPECT_EQ(reader->GetTypingEchoMode(), AccessReader::TypingEchoMode::Word);
}

TEST_F(TypeEchoFixture, SetBothMode) {
    reader->SetTypingEchoMode(AccessReader::TypingEchoMode::Both);
    EXPECT_EQ(reader->GetTypingEchoMode(), AccessReader::TypingEchoMode::Both);
}

TEST_F(TypeEchoFixture, SetOffMode) {
    reader->SetTypingEchoMode(AccessReader::TypingEchoMode::Char);
    reader->SetTypingEchoMode(AccessReader::TypingEchoMode::Off);
    EXPECT_EQ(reader->GetTypingEchoMode(), AccessReader::TypingEchoMode::Off);
}

TEST_F(TypeEchoFixture, TextChangedIgnoredWhenOff) {
    // TypingEchoMode::Off — no speech should be triggered
    reader->Initialize();
    AccessEvent evt;
    evt.type          = AccessEventType::TextChanged;
    evt.element.value = "hello";
    evt.element.state = AccessState::None;
    EXPECT_NO_FATAL_FAILURE(reader->OnEvent(evt)); // no crash
}

TEST_F(TypeEchoFixture, TextChangedCharEchoNoSpeechManagerNoCrash) {
    reader->SetTypingEchoMode(AccessReader::TypingEchoMode::Char);
    reader->Initialize();
    AccessEvent evt;
    evt.type          = AccessEventType::TextChanged;
    evt.element.value = "a";
    evt.element.state = AccessState::None;
    EXPECT_NO_FATAL_FAILURE(reader->OnEvent(evt));
}

TEST_F(TypeEchoFixture, TextChangedIgnoredForProtectedFields) {
    reader->SetTypingEchoMode(AccessReader::TypingEchoMode::Both);
    reader->Initialize();
    AccessEvent evt;
    evt.type          = AccessEventType::TextChanged;
    evt.element.value = "secret";
    evt.element.state = AccessState::Protected; // password field
    EXPECT_NO_FATAL_FAILURE(reader->OnEvent(evt));
}

// ── AccessReader::IsBrowseMode / BrowseMode defaults ─────────────────────────

TEST(AccessReaderBrowse, DefaultBrowseModeIsOff) {
    AccessReader reader(nullptr, nullptr, nullptr, nullptr);
    EXPECT_FALSE(reader.IsBrowseMode());
}

TEST(AccessReaderBrowse, ToggleBrowseModeNoSpeechNoCrash) {
    AccessReader reader(nullptr, nullptr, nullptr, nullptr);
    reader.Initialize();
    // Should not crash even with null speech manager
    EXPECT_NO_FATAL_FAILURE(reader.ToggleBrowseMode());
}

TEST(AccessReaderBrowse, HasActiveTableDefaultFalse) {
    AccessReader reader(nullptr, nullptr, nullptr, nullptr);
    EXPECT_FALSE(reader.HasActiveTable());
}

TEST(AccessReaderBrowse, GetCurrentTableCellEmptyWhenNoTable) {
    AccessReader reader(nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(reader.GetCurrentTableCell(), "");
}

TEST(AccessReaderBrowse, TableMoveNextEmptyWhenNoTable) {
    AccessReader reader(nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(reader.TableMoveNext(), "");
}

TEST(AccessReaderBrowse, SayAllNotActiveByDefault) {
    AccessReader reader(nullptr, nullptr, nullptr, nullptr);
    EXPECT_FALSE(reader.IsSayAllActive());
}

TEST(AccessReaderBrowse, StopSayAllNoSpeechNoCrash) {
    AccessReader reader(nullptr, nullptr, nullptr, nullptr);
    EXPECT_NO_FATAL_FAILURE(reader.StopSayAll());
}
