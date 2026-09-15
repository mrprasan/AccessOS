// AccessOS/src/Core/Events/EventQueue.h
//
// Thread-safe bounded event queue used by the event engine.
//
// Why: UIA event callbacks arrive on internal UIA threads and must
//      return immediately. Expensive processing (normalization,
//      semantic conversion) happens on worker threads that drain
//      this queue. A bounded queue prevents unbounded memory growth
//      if events arrive faster than they are consumed.
//
// Threading: Enqueue() is safe to call from any thread.
//            Dequeue() is intended for a single worker thread.
//            Both block only briefly on the internal mutex.

#pragma once

#include "IEventListener.h"

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <chrono>

namespace AccessOS {

class EventQueue {
public:
    // Maximum number of events held in the queue before dropping.
    // Prevents unbounded memory use if the worker cannot keep up.
    static constexpr size_t kMaxSize = 512;

    explicit EventQueue() = default;

    // Enqueue an event. If the queue is full, the oldest event is dropped
    // and a warning is logged. Never blocks the caller.
    // Thread-safe.
    void Enqueue(AccessEvent event);

    // Dequeue the next event. Blocks until an event is available or
    // the timeout expires.
    // Returns nullopt on timeout or if the queue is stopped.
    std::optional<AccessEvent> Dequeue(std::chrono::milliseconds timeout);

    // Signal all waiting Dequeue() calls to return nullopt and stop.
    void Stop();

    // Returns the current queue depth — for diagnostics only.
    size_t Size() const;

    bool IsStopped() const noexcept { return m_stopped; }

private:
    mutable std::mutex          m_mutex;
    std::condition_variable     m_condition;
    std::queue<AccessEvent>     m_queue;
    bool                        m_stopped{ false };
    size_t                      m_droppedCount{ 0 };  // Logged but not exposed
};

} // namespace AccessOS
