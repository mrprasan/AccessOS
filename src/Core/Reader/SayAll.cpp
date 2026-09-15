// SayAll.cpp — Continuous reading engine (ACCESSOS-031)
#include "SayAll.h"
#include <windows.h> // Sleep

namespace AccessOS {

SayAll::SayAll() = default;

void SayAll::SetCursor(Browse::VirtualCursor* cursor) {
    m_cursor = cursor;
}

void SayAll::SetSpeechManager(SpeechManager* speech) {
    m_speech = speech;
}

void SayAll::SetProgressCallback(SayAllProgressCallback cb) {
    m_progress = std::move(cb);
}

size_t SayAll::Run() {
    if (!m_cursor || !m_speech) return 0;
    if (!m_cursor->HasDocument()) return 0;

    m_stop.store(false);
    m_running.store(true);

    size_t spoken = 0;

    // We iterate by calling MoveNextElement after speaking each node
    // Start at current position
    size_t nodeCount = 0;
    {
        // Peek at document size without moving cursor
        // We can only get this via AnnouncePosition parsing, so we just iterate
        nodeCount = SIZE_MAX; // unknown, iterate until boundary
    }

    // Speak current node first
    {
        std::string text = m_cursor->ReadCurrentNode();
        if (!text.empty()) {
            m_speech->SpeakText(text, SpeechPriority::Normal, spoken == 0);
            ++spoken;
        }
    }

    // Advance through remaining nodes
    while (!m_stop.load()) {
        if (!m_cursor->MoveNextElement()) break; // reached end

        std::string text = m_cursor->ReadCurrentNode();
        if (!text.empty()) {
            m_speech->SpeakText(text, SpeechPriority::Normal, false);
            ++spoken;
        }

        // Invoke progress callback
        if (m_progress) {
            bool cont = m_progress(m_cursor->NodeIndex(), 0);
            if (!cont) break;
        }

        // Small yield between nodes so UI thread stays responsive
        ::Sleep(kInterNodeDelayMs);
    }

    m_running.store(false);
    return spoken;
}

void SayAll::Stop() noexcept {
    m_stop.store(true);
}

} // namespace AccessOS
