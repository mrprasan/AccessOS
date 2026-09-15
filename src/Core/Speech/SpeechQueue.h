// AccessOS/src/Core/Speech/SpeechQueue.h
//
// Priority-aware speech queue.
//
// Why: Speech requests arrive from multiple threads. The queue
//      serializes them, applies priority ordering, and allows
//      cancellation of stale utterances when higher-priority
//      speech arrives. The speech thread drains this queue.
//
// Threading: Enqueue() and Cancel() are thread-safe.
//            Dequeue() is called only by the speech thread.

#pragma once

#include "SpeechPriority.h"

#include <string>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <chrono>
#include <atomic>

namespace AccessOS {

struct SpeechRequest {
    std::string     text;
    SpeechPriority  priority  = SpeechPriority::Normal;
    bool            cancelPrevious = false; // If true, clears all lower-priority items
    uint64_t        id        = 0;          // Unique ID for cancellation
};

// Comparison for priority queue — higher priority value = higher urgency.
struct SpeechRequestCompare {
    bool operator()(const SpeechRequest& a, const SpeechRequest& b) const {
        return static_cast<uint8_t>(a.priority) < static_cast<uint8_t>(b.priority);
    }
};

class SpeechQueue {
public:
    SpeechQueue();

    // Enqueue a speech request. Thread-safe.
    // If request.cancelPrevious is true, lower-priority pending items are removed.
    void Enqueue(SpeechRequest request);

    // Dequeue the next request. Blocks until available or stopped.
    std::optional<SpeechRequest> Dequeue(std::chrono::milliseconds timeout);

    // Cancel all pending requests at or below the given priority.
    // Used when navigation moves to a new element (cancels stale Normal speech).
    void CancelUpTo(SpeechPriority maxPriority);

    // Cancel all pending requests. Does not stop the currently speaking utterance.
    void CancelAll();

    // Stop the queue — wakes the speech thread so it can exit.
    void Stop();

    bool IsStopped() const noexcept { return m_stopped.load(); }

    // Returns current queue depth. For diagnostics.
    size_t Size() const;

    // Returns the next unique request ID.
    uint64_t NextId() noexcept { return ++m_nextId; }

private:
    mutable std::mutex          m_mutex;
    std::condition_variable     m_condition;

    // Priority queue: highest priority dequeued first.
    std::priority_queue<
        SpeechRequest,
        std::vector<SpeechRequest>,
        SpeechRequestCompare>   m_queue;

    std::atomic<bool>           m_stopped{ false };
    std::atomic<uint64_t>       m_nextId{ 0 };
};

} // namespace AccessOS
