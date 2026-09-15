// AccessOS/src/Core/Events/EventQueue.cpp

#include "EventQueue.h"
#include "../Logging/Logger.h"

namespace AccessOS {

void EventQueue::Enqueue(AccessEvent event) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_stopped) return;

    if (m_queue.size() >= kMaxSize) {
        // Drop the oldest event to make room — log the drop.
        m_queue.pop();
        ++m_droppedCount;
        ACOS_LOG_WARNING("EventQueue",
            "Queue full — oldest event dropped. Total dropped: " +
            std::to_string(m_droppedCount));
    }

    m_queue.push(std::move(event));
    m_condition.notify_one();
}

std::optional<AccessEvent> EventQueue::Dequeue(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(m_mutex);

    const bool signaled = m_condition.wait_for(lock, timeout, [this] {
        return !m_queue.empty() || m_stopped;
    });

    if (!signaled || m_stopped || m_queue.empty()) {
        return std::nullopt;
    }

    AccessEvent event = std::move(m_queue.front());
    m_queue.pop();
    return event;
}

void EventQueue::Stop() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stopped = true;
    m_condition.notify_all();
}

size_t EventQueue::Size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}

} // namespace AccessOS
