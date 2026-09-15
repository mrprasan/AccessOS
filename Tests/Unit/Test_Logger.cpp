// Tests/Unit/Test_Logger.cpp
// Unit tests for AccessOS::Logger.

#include <gtest/gtest.h>
#include "Logging/Logger.h"

#include <vector>
#include <string>

using namespace AccessOS;

// ─── Test sink ───────────────────────────────────────────────────────────────

class CaptureSink final : public ILogSink {
public:
    void Write(const LogEntry& entry) override {
        entries.push_back(entry);
    }

    std::vector<LogEntry> entries;
};

// ─── Fixture ─────────────────────────────────────────────────────────────────

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_sink = std::make_shared<CaptureSink>();
        Logger::Instance().SetSink(m_sink);
        Logger::Instance().SetLevel(LogLevel::Debug);
    }

    void TearDown() override {
        // Restore default sink so other tests are unaffected.
        Logger::Instance().SetSink(nullptr);
        Logger::Instance().SetLevel(LogLevel::Info);
    }

    std::shared_ptr<CaptureSink> m_sink;
};

// ─── Tests ───────────────────────────────────────────────────────────────────

TEST_F(LoggerTest, InfoMessageIsRecorded) {
    ACOS_LOG_INFO("TestComponent", "hello from test");

    ASSERT_EQ(m_sink->entries.size(), 1u);
    EXPECT_EQ(m_sink->entries[0].level,     LogLevel::Info);
    EXPECT_EQ(m_sink->entries[0].component, "TestComponent");
    EXPECT_EQ(m_sink->entries[0].message,   "hello from test");
}

TEST_F(LoggerTest, WarningMessageIsRecorded) {
    ACOS_LOG_WARNING("TestComponent", "a warning");

    ASSERT_EQ(m_sink->entries.size(), 1u);
    EXPECT_EQ(m_sink->entries[0].level, LogLevel::Warning);
}

TEST_F(LoggerTest, ErrorMessageIsRecorded) {
    ACOS_LOG_ERROR("TestComponent", "an error");

    ASSERT_EQ(m_sink->entries.size(), 1u);
    EXPECT_EQ(m_sink->entries[0].level, LogLevel::Error);
}

TEST_F(LoggerTest, CriticalMessageIsRecorded) {
    ACOS_LOG_CRITICAL("TestComponent", "critical failure");

    ASSERT_EQ(m_sink->entries.size(), 1u);
    EXPECT_EQ(m_sink->entries[0].level, LogLevel::Critical);
}

TEST_F(LoggerTest, DebugMessageRecordedWhenLevelIsDebug) {
    Logger::Instance().SetLevel(LogLevel::Debug);
    ACOS_LOG_DEBUG("TestComponent", "debug message");

    ASSERT_EQ(m_sink->entries.size(), 1u);
    EXPECT_EQ(m_sink->entries[0].level, LogLevel::Debug);
}

TEST_F(LoggerTest, DebugMessageSuppressedWhenLevelIsInfo) {
    Logger::Instance().SetLevel(LogLevel::Info);
    ACOS_LOG_DEBUG("TestComponent", "should be suppressed");

    EXPECT_EQ(m_sink->entries.size(), 0u);
}

TEST_F(LoggerTest, LowerLevelMessagesSuppressed) {
    Logger::Instance().SetLevel(LogLevel::Error);

    ACOS_LOG_DEBUG  ("TC", "debug");
    ACOS_LOG_INFO   ("TC", "info");
    ACOS_LOG_WARNING("TC", "warning");

    EXPECT_EQ(m_sink->entries.size(), 0u);

    ACOS_LOG_ERROR("TC", "error");
    EXPECT_EQ(m_sink->entries.size(), 1u);
}

TEST_F(LoggerTest, NullSinkProducesNoOutput) {
    Logger::Instance().SetSink(nullptr);
    // Must not crash.
    ACOS_LOG_INFO("TC", "message with null sink");
    // No assertion needed — passing without crash is the requirement.
}

TEST_F(LoggerTest, MultipleMessagesRecordedInOrder) {
    ACOS_LOG_INFO   ("TC", "first");
    ACOS_LOG_WARNING("TC", "second");
    ACOS_LOG_ERROR  ("TC", "third");

    ASSERT_EQ(m_sink->entries.size(), 3u);
    EXPECT_EQ(m_sink->entries[0].message, "first");
    EXPECT_EQ(m_sink->entries[1].message, "second");
    EXPECT_EQ(m_sink->entries[2].message, "third");
}
