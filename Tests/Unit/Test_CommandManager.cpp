// Tests/Unit/Test_CommandManager.cpp
// Unit tests for CommandManager and LambdaCommand.
// No live UIA, no keyboard, no speech required.

#include <gtest/gtest.h>
#include "Commands/CommandManager.h"

#include <vector>
#include <string>
#include <atomic>
#include <thread>

using namespace AccessOS;

// ─── CommandManager — basic registration ─────────────────────────────────────

TEST(CommandManagerTest, RegisterAndGetById) {
    CommandManager mgr;
    mgr.Register("nav.next", "Next Element", [](){});
    EXPECT_NE(mgr.Get("nav.next"), nullptr);
}

TEST(CommandManagerTest, GetUnknownReturnsNull) {
    CommandManager mgr;
    EXPECT_EQ(mgr.Get("does.not.exist"), nullptr);
}

TEST(CommandManagerTest, CountAfterRegister) {
    CommandManager mgr;
    EXPECT_EQ(mgr.Count(), 0u);
    mgr.Register("a", "A", [](){});
    mgr.Register("b", "B", [](){});
    EXPECT_EQ(mgr.Count(), 2u);
}

TEST(CommandManagerTest, RegisterReplacesDuplicate) {
    CommandManager mgr;
    bool first  = false;
    bool second = false;
    mgr.Register("cmd", "First",  [&first]()  { first  = true; });
    mgr.Register("cmd", "Second", [&second]() { second = true; });
    EXPECT_EQ(mgr.Count(), 1u);   // Only one entry.
    mgr.Execute("cmd");
    EXPECT_FALSE(first);   // First command was replaced.
    EXPECT_TRUE(second);
}

TEST(CommandManagerTest, ExecuteInvokesCommand) {
    CommandManager mgr;
    bool ran = false;
    mgr.Register("test.run", "Run", [&ran](){ ran = true; });
    const bool result = mgr.Execute("test.run");
    EXPECT_TRUE(result);
    EXPECT_TRUE(ran);
}

TEST(CommandManagerTest, ExecuteUnknownReturnsFalse) {
    CommandManager mgr;
    EXPECT_FALSE(mgr.Execute("ghost.command"));
}

TEST(CommandManagerTest, AllIdsContainsRegistered) {
    CommandManager mgr;
    mgr.Register("x", "X", [](){});
    mgr.Register("y", "Y", [](){});
    const auto ids = mgr.AllIds();
    EXPECT_EQ(ids.size(), 2u);
    // Both IDs present (order not guaranteed).
    bool hasX = false, hasY = false;
    for (const auto& id : ids) {
        if (id == "x") hasX = true;
        if (id == "y") hasY = true;
    }
    EXPECT_TRUE(hasX);
    EXPECT_TRUE(hasY);
}

// ─── LambdaCommand ────────────────────────────────────────────────────────────

TEST(LambdaCommandTest, IdAndDisplayName) {
    LambdaCommand cmd("speech.stop", "Stop Speech", [](){});
    EXPECT_STREQ(cmd.Id(), "speech.stop");
    EXPECT_STREQ(cmd.DisplayName(), "Stop Speech");
}

TEST(LambdaCommandTest, ExecuteCallsLambda) {
    int count = 0;
    LambdaCommand cmd("counter", "Counter", [&count](){ ++count; });
    cmd.Execute();
    cmd.Execute();
    EXPECT_EQ(count, 2);
}

TEST(LambdaCommandTest, NullLambdaDoesNotCrash) {
    LambdaCommand cmd("null.cmd", "Null", nullptr);
    EXPECT_NO_THROW(cmd.Execute());
}

// ─── CommandManager — ICommand pointer overload ───────────────────────────────

TEST(CommandManagerTest, RegisterSharedPtrCommand) {
    CommandManager mgr;
    auto cmd = std::make_shared<LambdaCommand>("ptr.cmd", "Ptr", [](){});
    mgr.Register(cmd);
    EXPECT_NE(mgr.Get("ptr.cmd"), nullptr);
}

TEST(CommandManagerTest, RegisterNullptrIsIgnored) {
    CommandManager mgr;
    mgr.Register(nullptr);
    EXPECT_EQ(mgr.Count(), 0u);
}

// ─── Thread safety: concurrent executions ────────────────────────────────────

TEST(CommandManagerTest, ConcurrentExecuteIsSafe) {
    CommandManager mgr;
    std::atomic<int> counter{ 0 };
    mgr.Register("atomic.inc", "Inc", [&counter](){ counter.fetch_add(1); });

    constexpr int kThreads = 8;
    constexpr int kIter    = 100;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&mgr, kIter]() {
            for (int j = 0; j < kIter; ++j) {
                mgr.Execute("atomic.inc");
            }
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(counter.load(), kThreads * kIter);
}
