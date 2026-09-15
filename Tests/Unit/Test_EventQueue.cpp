// Tests/Unit/Test_EventQueue.cpp
// Unit tests for EventQueue — thread-safe bounded queue.
// No UIA session required.

#include <gtest/gtest.h>
#include "Events/EventQueue.h"

#include <thread>
#include <vector>
#include <atomic>

using namespace AccessOS;

// ─── Helpers ─────────────────────────────────────────────────────────────────

static AccessEvent MakeEvent(AccessEventType type, const std::string& name = {}) {
    AccessEvent e;
    e.type = type;
    e.element.name = name;
    e.timestampMs  = 42;
    return e;
}

// ─── Basic enqueue / dequeue ─────────────────────────────────────────────────

TEST(EventQueueTest, EnqueuedEventIsDequeued) {
    EventQueue q;
    q.Enqueue(MakeEvent(AccessEventType::FocusChanged, "btn"));

    auto result = q.Dequeue(std::chrono::milliseconds(100));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->type, AccessEventType::FocusChanged);
    EXPECT_EQ(result->element.name, "btn");
}

TEST(EventQueueTest, DequeueTimesOutWhenEmpty) {
    EventQueue q;
    auto result = q.Dequeue(std::chrono::milliseconds(50));
    EXPECT_FALSE(result.has_value());
}

TEST(EventQueueTest, MultipleEventsDequeueInOrder) {
    EventQueue q;
    q.Enqueue(MakeEvent(AccessEventType::FocusChanged,   "first"));
    q.Enqueue(MakeEvent(AccessEventType::NameChanged,    "second"));
    q.Enqueue(MakeEvent(AccessEventType::ValueChanged,   "third"));

    auto e1 = q.Dequeue(std::chrono::milliseconds(100));
    auto e2 = q.Dequeue(std::chrono::milliseconds(100));
    auto e3 = q.Dequeue(std::chrono::milliseconds(100));

    ASSERT_TRUE(e1.has_value());
    ASSERT_TRUE(e2.has_value());
    ASSERT_TRUE(e3.has_value());
    EXPECT_EQ(e1->element.name, "first");
    EXPECT_EQ(e2->element.name, "second");
    EXPECT_EQ(e3->element.name, "third");
}

// ─── Stop behaviour ──────────────────────────────────────────────────────────

TEST(EventQueueTest, StopCausesDequeueToReturnNullopt) {
    EventQueue q;
    q.Stop();
    auto result = q.Dequeue(std::chrono::milliseconds(100));
    EXPECT_FALSE(result.has_value());
}

TEST(EventQueueTest, EnqueueAfterStopIsIgnored) {
    EventQueue q;
    q.Stop();
    q.Enqueue(MakeEvent(AccessEventType::FocusChanged));
    EXPECT_EQ(q.Size(), 0u);
}

TEST(EventQueueTest, StopWakesBlockedDequeue) {
    EventQueue q;
    bool woken = false;

    std::thread consumer([&] {
        auto result = q.Dequeue(std::chrono::milliseconds(5000));
        woken = !result.has_value();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    q.Stop();
    consumer.join();

    EXPECT_TRUE(woken);
}

// ─── Bounded queue / drop behaviour ──────────────────────────────────────────

TEST(EventQueueTest, QueueDropsOldestWhenFull) {
    EventQueue q;

    // Fill to max + 1
    for (size_t i = 0; i <= EventQueue::kMaxSize; ++i) {
        q.Enqueue(MakeEvent(AccessEventType::FocusChanged,
                            "event_" + std::to_string(i)));
    }

    // Queue size must not exceed kMaxSize
    EXPECT_LE(q.Size(), EventQueue::kMaxSize);
}

// ─── Size ─────────────────────────────────────────────────────────────────────

TEST(EventQueueTest, SizeReflectsQueueDepth) {
    EventQueue q;
    EXPECT_EQ(q.Size(), 0u);

    q.Enqueue(MakeEvent(AccessEventType::FocusChanged));
    EXPECT_EQ(q.Size(), 1u);

    q.Enqueue(MakeEvent(AccessEventType::NameChanged));
    EXPECT_EQ(q.Size(), 2u);

    q.Dequeue(std::chrono::milliseconds(100));
    EXPECT_EQ(q.Size(), 1u);
}

// ─── Concurrent enqueue ───────────────────────────────────────────────────────

TEST(EventQueueTest, ConcurrentEnqueueIsThreadSafe) {
    EventQueue q;
    constexpr int kThreads = 4;
    constexpr int kPerThread = 50;
    std::atomic<int> enqueued{ 0 };

    std::vector<std::thread> producers;
    for (int t = 0; t < kThreads; ++t) {
        producers.emplace_back([&] {
            for (int i = 0; i < kPerThread; ++i) {
                q.Enqueue(MakeEvent(AccessEventType::FocusChanged));
                ++enqueued;
            }
        });
    }
    for (auto& p : producers) p.join();

    // All enqueued events must be in the queue (up to kMaxSize)
    EXPECT_LE(q.Size(), EventQueue::kMaxSize);
    EXPECT_EQ(enqueued.load(), kThreads * kPerThread);
}
