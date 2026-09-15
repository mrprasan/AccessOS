// AccessOS/src/Core/Speech/SpeechQueue.cpp

#include "SpeechQueue.h"

namespace AccessOS {

SpeechQueue::SpeechQueue() = default;

void SpeechQueue::Enqueue(SpeechRequest request) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_stopped.load()) return;

    if (request.cancelPrevious) {
        // Drain all items at lower or equal priority.
        // std::priority_queue does not support iteration, so rebuild.
        std::vector<SpeechRequest> keep;
        while (!m_queue.empty()) {
            auto top = m_queue.top();
            m_queue.pop();
            if (static_cast<uint8_t>(top.priority) >
                static_cast<uint8_t>(request.priority)) {
                keep.push_back(std::move(top));
            }
        }
        for (auto& r : keep) m_queue.push(std::move(r));
    }

    m_queue.push(std::move(request));
    m_condition.notify_one();
}

std::optional<SpeechRequest> SpeechQueue::Dequeue(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(m_mutex);

    const bool signaled = m_condition.wait_for(lock, timeout, [this] {
        return !m_queue.empty() || m_stopped.load();
    });

    if (!signaled || m_stopped.load() || m_queue.empty()) {
        return std::nullopt;
    }

    SpeechRequest req = m_queue.top();
    m_queue.pop();
    return req;
}

void SpeechQueue::CancelUpTo(SpeechPriority maxPriority) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<SpeechRequest> keep;
    while (!m_queue.empty()) {
        auto top = m_queue.top();
        m_queue.pop();
        if (static_cast<uint8_t>(top.priority) >
            static_cast<uint8_t>(maxPriority)) {
            keep.push_back(std::move(top));
        }
    }
    for (auto& r : keep) m_queue.push(std::move(r));
}

void SpeechQueue::CancelAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    while (!m_queue.empty()) m_queue.pop();
}

void SpeechQueue::Stop() {
    m_stopped.store(true);
    m_condition.notify_all();
}

size_t SpeechQueue::Size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}

} // namespace AccessOS
