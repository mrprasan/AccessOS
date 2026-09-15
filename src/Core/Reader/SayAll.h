#pragma once
// SayAll.h — Continuous reading engine (ACCESSOS-031)
//
// Iterates VirtualDocument nodes from the current cursor position to end,
// speaking each node's text sequentially.  Interruptible by calling Stop().
//
// Threading: Run() must be called on a dedicated background thread.
//            Stop() is thread-safe.

#include "../Browse/VirtualCursor.h"
#include "../Speech/SpeechManager.h"
#include <atomic>
#include <memory>
#include <functional>

namespace AccessOS {

// Callback fired after each node is spoken (nodeIndex = 0-based position)
// Return false to abort Say All early.
using SayAllProgressCallback = std::function<bool(size_t nodeIndex, size_t total)>;

class SayAll {
public:
    SayAll();

    // Set the cursor and speech manager (must be called before Run)
    void SetCursor(Browse::VirtualCursor* cursor);
    void SetSpeechManager(SpeechManager* speech);

    // Optional progress callback (called after each node is queued)
    void SetProgressCallback(SayAllProgressCallback cb);

    // Start reading from the cursor's current position to end of document.
    // Blocks until complete, interrupted, or cursor has no document.
    // Returns the number of nodes spoken.
    size_t Run();

    // Signal Run() to stop after the current node completes.
    // Thread-safe; safe to call from any thread including UI.
    void Stop() noexcept;

    // True while Run() is executing.
    bool IsRunning() const noexcept { return m_running.load(); }

private:
    Browse::VirtualCursor*       m_cursor  = nullptr;
    SpeechManager*               m_speech  = nullptr;
    SayAllProgressCallback       m_progress;
    std::atomic<bool>            m_running{ false };
    std::atomic<bool>            m_stop{ false };

    // Inter-node delay in ms (allows speech queue to drain between nodes)
    static constexpr uint32_t kInterNodeDelayMs = 80;
};

} // namespace AccessOS
