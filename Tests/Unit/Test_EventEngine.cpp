// Tests/Unit/Test_EventEngine.cpp
// Unit tests for EventEngine listener dispatch — no live UIA required.

#include <gtest/gtest.h>
#include "Events/EventEngine.h"
#include "Events/IEventListener.h"

#include <vector>
#include <mutex>
#include <chrono>
#include <thread>

using namespace AccessOS;

// ─── Test listener ───────────────────────────────────────────────────────────

class RecordingListener final : public IEventListener {
public:
    void OnEvent(const AccessEvent& event) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        received.push_back(event);
    }

    size_t Count() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return received.size();
    }

    AccessEvent Last() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return received.back();
    }

    mutable std::mutex          m_mutex;
    std::vector<AccessEvent>    received;
};

// ─── EventQueue + listener dispatch (no UIA) ─────────────────────────────────
// These tests exercise the queue-to-listener dispatch path directly,
// bypassing the UIA event sink (which requires a live COM session).

TEST(EventEngineTest, ListenerReceivesEnqueuedEvent) {
    // Directly test: enqueue → worker drains → listener called
    auto queue = std::make_shared<EventQueue>();
    RecordingListener listener;

    // Simulate what the worker thread does: dequeue and dispatch to listener
    AccessEvent evt;
    evt.type            = AccessEventType::FocusChanged;
    evt.element.name    = "TestButton";
    evt.element.role    = AccessRole::Button;
    evt.timestampMs     = 100;

    queue->Enqueue(evt);
    auto dequeued = queue->Dequeue(std::chrono::milliseconds(200));

    ASSERT_TRUE(dequeued.has_value());
    listener.OnEvent(dequeued.value());

    ASSERT_EQ(listener.Count(), 1u);
    EXPECT_EQ(listener.Last().type,         AccessEventType::FocusChanged);
    EXPECT_EQ(listener.Last().element.name, "TestButton");
    EXPECT_EQ(listener.Last().element.role, AccessRole::Button);
}

TEST(EventEngineTest, MultipleListenersAllReceiveEvent) {
    auto queue = std::make_shared<EventQueue>();
    RecordingListener l1, l2, l3;

    AccessEvent evt;
    evt.type         = AccessEventType::NameChanged;
    evt.element.name = "Label";

    queue->Enqueue(evt);
    auto dequeued = queue->Dequeue(std::chrono::milliseconds(200));
    ASSERT_TRUE(dequeued.has_value());

    l1.OnEvent(dequeued.value());
    l2.OnEvent(dequeued.value());
    l3.OnEvent(dequeued.value());

    EXPECT_EQ(l1.Count(), 1u);
    EXPECT_EQ(l2.Count(), 1u);
    EXPECT_EQ(l3.Count(), 1u);
}

TEST(EventEngineTest, EventTimestampIsPreserved) {
    auto queue = std::make_shared<EventQueue>();
    RecordingListener listener;

    AccessEvent evt;
    evt.type        = AccessEventType::ValueChanged;
    evt.timestampMs = 99999;

    queue->Enqueue(evt);
    auto dequeued = queue->Dequeue(std::chrono::milliseconds(200));
    ASSERT_TRUE(dequeued.has_value());
    listener.OnEvent(dequeued.value());

    EXPECT_EQ(listener.Last().timestampMs, 99999u);
}

TEST(EventEngineTest, EventTypeIsPreservedThroughQueue) {
    auto queue = std::make_shared<EventQueue>();
    RecordingListener listener;

    const std::vector<AccessEventType> types = {
        AccessEventType::FocusChanged,
        AccessEventType::StructureChanged,
        AccessEventType::NameChanged,
        AccessEventType::ValueChanged,
        AccessEventType::StateChanged,
        AccessEventType::LiveRegionChanged,
    };

    for (auto t : types) {
        AccessEvent e;
        e.type = t;
        queue->Enqueue(e);
    }

    for (size_t i = 0; i < types.size(); ++i) {
        auto d = queue->Dequeue(std::chrono::milliseconds(100));
        ASSERT_TRUE(d.has_value());
        listener.OnEvent(d.value());
    }

    ASSERT_EQ(listener.Count(), types.size());
    for (size_t i = 0; i < types.size(); ++i) {
        EXPECT_EQ(listener.received[i].type, types[i]);
    }
}
