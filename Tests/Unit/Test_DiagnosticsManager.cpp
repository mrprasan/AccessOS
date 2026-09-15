// AccessOS/Tests/Unit/Test_DiagnosticsManager.cpp
//
// Unit tests for DiagnosticsCounters, FileSink, and DiagnosticsManager.
//
// Coverage:
//   - Counter increment and read
//   - Counter reset + uptime reset
//   - Uptime monotonically increases
//   - FileSink open/close/IsOpen
//   - FileSink writes formatted lines to a temp file
//   - DiagnosticsManager SetLogFile / CloseLogFile / IsLoggingToFile
//   - DiagnosticsManager GetSnapshot reflects counter state
//   - DiagnosticsManager ResetCounters delegates to singleton
//   - Static convenience wrappers RecordXxx

#include <gtest/gtest.h>
#include "Diagnostics/DiagnosticsCounters.h"
#include "Diagnostics/FileSink.h"
#include "Diagnostics/DiagnosticsManager.h"
#include "Logging/Logger.h"

#include <thread>
#include <chrono>
#include <fstream>
#include <string>
#include <filesystem>

using namespace AccessOS;

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string TempPath(const std::string& name) {
    // Write to the system temp directory so we don't pollute the source tree.
    return (std::filesystem::temp_directory_path() / name).string();
}

static bool FileContains(const std::string& path, const std::string& substr) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::string line;
    while (std::getline(f, line)) {
        if (line.find(substr) != std::string::npos) return true;
    }
    return false;
}

// ─── DiagnosticsCounters ──────────────────────────────────────────────────────

TEST(DiagnosticsCounters, IncrementEventsProcessed) {
    auto& c = DiagnosticsCounters::Instance();
    c.Reset();
    EXPECT_EQ(c.EventsProcessed(), 0u);
    c.IncrementEventsProcessed();
    c.IncrementEventsProcessed();
    EXPECT_EQ(c.EventsProcessed(), 2u);
    c.Reset();
}

TEST(DiagnosticsCounters, IncrementFocusChanges) {
    auto& c = DiagnosticsCounters::Instance();
    c.Reset();
    c.IncrementFocusChanges();
    EXPECT_EQ(c.FocusChanges(), 1u);
    c.Reset();
}

TEST(DiagnosticsCounters, IncrementSpeechUtterances) {
    auto& c = DiagnosticsCounters::Instance();
    c.Reset();
    c.IncrementSpeechUtterances();
    c.IncrementSpeechUtterances();
    c.IncrementSpeechUtterances();
    EXPECT_EQ(c.SpeechUtterances(), 3u);
    c.Reset();
}

TEST(DiagnosticsCounters, IncrementContextSwitches) {
    auto& c = DiagnosticsCounters::Instance();
    c.Reset();
    c.IncrementContextSwitches();
    EXPECT_EQ(c.ContextSwitches(), 1u);
    c.Reset();
}

TEST(DiagnosticsCounters, IncrementSpeechCancels) {
    auto& c = DiagnosticsCounters::Instance();
    c.Reset();
    c.IncrementSpeechCancels();
    c.IncrementSpeechCancels();
    EXPECT_EQ(c.SpeechCancels(), 2u);
    c.Reset();
}

TEST(DiagnosticsCounters, ResetClearsAll) {
    auto& c = DiagnosticsCounters::Instance();
    c.IncrementEventsProcessed();
    c.IncrementFocusChanges();
    c.IncrementSpeechUtterances();
    c.IncrementContextSwitches();
    c.IncrementSpeechCancels();
    c.Reset();
    EXPECT_EQ(c.EventsProcessed(),  0u);
    EXPECT_EQ(c.FocusChanges(),     0u);
    EXPECT_EQ(c.SpeechUtterances(), 0u);
    EXPECT_EQ(c.ContextSwitches(),  0u);
    EXPECT_EQ(c.SpeechCancels(),    0u);
}

TEST(DiagnosticsCounters, UptimeIncreases) {
    auto& c = DiagnosticsCounters::Instance();
    c.Reset();
    const uint64_t t0 = c.UptimeMs();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    const uint64_t t1 = c.UptimeMs();
    EXPECT_GT(t1, t0);
    c.Reset();
}

TEST(DiagnosticsCounters, UptimeResetsOnReset) {
    auto& c = DiagnosticsCounters::Instance();
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    c.Reset();
    const uint64_t t = c.UptimeMs();
    EXPECT_LT(t, 30u);  // Should be near-zero after reset.
}

TEST(DiagnosticsCounters, ThreadSafeIncrement) {
    auto& c = DiagnosticsCounters::Instance();
    c.Reset();
    constexpr int kThreads = 4;
    constexpr int kPerThread = 100;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&c]() {
            for (int j = 0; j < kPerThread; ++j)
                c.IncrementEventsProcessed();
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(c.EventsProcessed(), static_cast<uint64_t>(kThreads * kPerThread));
    c.Reset();
}

// ─── FileSink ─────────────────────────────────────────────────────────────────

TEST(FileSink, IsClosedByDefault) {
    FileSink sink;
    EXPECT_FALSE(sink.IsOpen());
}

TEST(FileSink, OpenCreatesFile) {
    const std::string path = TempPath("acos_test_filesink.log");
    std::filesystem::remove(path);
    FileSink sink;
    EXPECT_TRUE(sink.Open(path));
    EXPECT_TRUE(sink.IsOpen());
    sink.Close();
    EXPECT_FALSE(sink.IsOpen());
    std::filesystem::remove(path);
}

TEST(FileSink, WriteAppendsLine) {
    const std::string path = TempPath("acos_test_write.log");
    std::filesystem::remove(path);

    FileSink sink;
    ASSERT_TRUE(sink.Open(path));

    LogEntry entry;
    entry.level     = LogLevel::Info;
    entry.component = "TestComp";
    entry.message   = "hello diagnostics";
    entry.file      = "test.cpp";
    entry.line      = 1;
    sink.Write(entry);
    sink.Close();

    EXPECT_TRUE(FileContains(path, "INFO"));
    EXPECT_TRUE(FileContains(path, "TestComp"));
    EXPECT_TRUE(FileContains(path, "hello diagnostics"));

    std::filesystem::remove(path);
}

TEST(FileSink, WriteAfterCloseIsIgnored) {
    const std::string path = TempPath("acos_test_closed.log");
    std::filesystem::remove(path);

    FileSink sink;
    ASSERT_TRUE(sink.Open(path));
    sink.Close();

    // Write after close must not crash.
    LogEntry entry;
    entry.level     = LogLevel::Error;
    entry.component = "X";
    entry.message   = "should not appear";
    EXPECT_NO_THROW(sink.Write(entry));

    std::filesystem::remove(path);
}

TEST(FileSink, MultipleWritesDifferentLevels) {
    const std::string path = TempPath("acos_test_levels.log");
    std::filesystem::remove(path);

    FileSink sink;
    ASSERT_TRUE(sink.Open(path));

    for (auto lvl : { LogLevel::Debug, LogLevel::Info, LogLevel::Warning,
                      LogLevel::Error, LogLevel::Critical }) {
        LogEntry e;
        e.level     = lvl;
        e.component = "LevelTest";
        e.message   = "msg";
        sink.Write(e);
    }
    sink.Close();

    EXPECT_TRUE(FileContains(path, "DEBUG"));
    EXPECT_TRUE(FileContains(path, "INFO"));
    EXPECT_TRUE(FileContains(path, "WARN"));
    EXPECT_TRUE(FileContains(path, "ERROR"));
    EXPECT_TRUE(FileContains(path, "CRIT"));

    std::filesystem::remove(path);
}

// ─── DiagnosticsManager ───────────────────────────────────────────────────────

TEST(DiagnosticsManager, IsNotLoggingToFileByDefault) {
    DiagnosticsManager mgr;
    EXPECT_FALSE(mgr.IsLoggingToFile());
}

TEST(DiagnosticsManager, SetLogFileOpensFile) {
    const std::string path = TempPath("acos_test_mgr.log");
    std::filesystem::remove(path);

    DiagnosticsManager mgr;
    EXPECT_TRUE(mgr.SetLogFile(path));
    EXPECT_TRUE(mgr.IsLoggingToFile());

    mgr.CloseLogFile();
    EXPECT_FALSE(mgr.IsLoggingToFile());

    std::filesystem::remove(path);
}

TEST(DiagnosticsManager, SetLogFileInvalidPathReturnsFalse) {
    DiagnosticsManager mgr;
    // An invalid path (impossible directory) should fail gracefully.
    bool ok = mgr.SetLogFile("Z:\\nonexistent\\deep\\path\\acos.log");
    EXPECT_FALSE(ok);
    EXPECT_FALSE(mgr.IsLoggingToFile());
}

TEST(DiagnosticsManager, GetSnapshotReflectsCounters) {
    DiagnosticsCounters::Instance().Reset();

    DiagnosticsManager mgr;
    DiagnosticsManager::RecordEvent();
    DiagnosticsManager::RecordEvent();
    DiagnosticsManager::RecordFocusChange();
    DiagnosticsManager::RecordSpeech();
    DiagnosticsManager::RecordSpeech();
    DiagnosticsManager::RecordSpeech();
    DiagnosticsManager::RecordContextSwitch();
    DiagnosticsManager::RecordSpeechCancel();

    DiagnosticsSnapshot snap = mgr.GetSnapshot();
    EXPECT_EQ(snap.eventsProcessed,  2u);
    EXPECT_EQ(snap.focusChanges,     1u);
    EXPECT_EQ(snap.speechUtterances, 3u);
    EXPECT_EQ(snap.contextSwitches,  1u);
    EXPECT_EQ(snap.speechCancels,    1u);

    DiagnosticsCounters::Instance().Reset();
}

TEST(DiagnosticsManager, ResetCountersClearsAll) {
    DiagnosticsManager::RecordEvent();
    DiagnosticsManager::RecordFocusChange();

    DiagnosticsManager mgr;
    mgr.ResetCounters();

    DiagnosticsSnapshot snap = mgr.GetSnapshot();
    EXPECT_EQ(snap.eventsProcessed, 0u);
    EXPECT_EQ(snap.focusChanges,    0u);
}

TEST(DiagnosticsManager, UptimeInSnapshot) {
    DiagnosticsCounters::Instance().Reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    DiagnosticsManager mgr;
    DiagnosticsSnapshot snap = mgr.GetSnapshot();
    EXPECT_GE(snap.uptimeMs, 20u);

    DiagnosticsCounters::Instance().Reset();
}

TEST(DiagnosticsManager, StaticRecordEventIncrementsCounter) {
    DiagnosticsCounters::Instance().Reset();
    DiagnosticsManager::RecordEvent();
    DiagnosticsManager::RecordEvent();
    EXPECT_EQ(DiagnosticsCounters::Instance().EventsProcessed(), 2u);
    DiagnosticsCounters::Instance().Reset();
}

TEST(DiagnosticsManager, StaticRecordSpeechIncrementsCounter) {
    DiagnosticsCounters::Instance().Reset();
    DiagnosticsManager::RecordSpeech();
    EXPECT_EQ(DiagnosticsCounters::Instance().SpeechUtterances(), 1u);
    DiagnosticsCounters::Instance().Reset();
}

TEST(DiagnosticsManager, LogFileReceivesLoggerOutput) {
    const std::string path = TempPath("acos_test_logger_route.log");
    std::filesystem::remove(path);

    DiagnosticsManager mgr;
    ASSERT_TRUE(mgr.SetLogFile(path));

    ACOS_LOG_INFO("DiagTest", "pipeline log routing test");

    mgr.CloseLogFile();

    EXPECT_TRUE(FileContains(path, "DiagTest"));
    EXPECT_TRUE(FileContains(path, "pipeline log routing test"));

    std::filesystem::remove(path);
}
