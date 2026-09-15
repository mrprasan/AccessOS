// AccessOS/src/Core/Events/EventEngine.h
//
// Central event engine — owns the event queue, worker thread,
// UIA event sink registration, and listener dispatch.
//
// Why: All accessibility events must flow through one controlled pipeline.
//      This engine ensures:
//       - UIA callbacks return immediately (queue-based)
//       - Normalization and dispatch happen on a dedicated worker thread
//       - Listeners are decoupled from providers
//       - Event dropping is bounded and logged
//
// Threading:
//   - Initialize() / Shutdown() must be called from the same thread
//     that owns the COM MTA apartment.
//   - The worker thread is owned entirely by EventEngine.
//   - Listeners receive events on the worker thread — they must not block.
//
// Must NOT: perform speech, navigation, or UI updates directly.

#pragma once

#include "EventQueue.h"
#include "UIAEventSink.h"
#include "IEventListener.h"
#include "../Error/AccessError.h"
#include "../Accessibility/UIAutomation/UIAIncludes.h"

#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

namespace AccessOS {

class EventEngine {
public:
    EventEngine();
    ~EventEngine();

    // Initialize the engine and register UIA event handlers.
    // Must be called after CoInitializeEx(MTA).
    // automation — non-owning pointer to the active IUIAutomation instance.
    Result<void> Initialize(IUIAutomation* automation);

    // Stop the worker thread and unregister all UIA event handlers.
    void Shutdown();

    bool IsRunning() const noexcept { return m_running.load(); }

    // Register a listener to receive normalized events.
    // Thread-safe. Listeners are called on the worker thread.
    void AddListener(IEventListener* listener);

    // Remove a previously registered listener.
    // Thread-safe.
    void RemoveListener(IEventListener* listener);

    // Returns the current event queue depth — for diagnostics.
    size_t QueueDepth() const;

private:
    // Worker thread entry point — drains the queue and dispatches events.
    void WorkerThread();

    // Dispatches a single event to all registered listeners.
    void DispatchEvent(const AccessEvent& event);

    IUIAutomation*                  m_automation{ nullptr };  // Non-owning
    std::shared_ptr<EventQueue>     m_queue;

    // COM sink — reference counted, registered with UIA.
    UIAEventSink*                   m_sink{ nullptr };

    std::thread                     m_worker;
    std::atomic<bool>               m_running{ false };

    mutable std::mutex              m_listenerMutex;
    std::vector<IEventListener*>    m_listeners;
};

} // namespace AccessOS
