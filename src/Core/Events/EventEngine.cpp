// AccessOS/src/Core/Events/EventEngine.cpp

#include "EventEngine.h"
#include "../Logging/Logger.h"

#include <sstream>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace AccessOS {

static constexpr const char* kComponent = "EventEngine";

// ─── Constructor / Destructor ────────────────────────────────────────────────

EventEngine::EventEngine()
    : m_queue(std::make_shared<EventQueue>())
{
}

EventEngine::~EventEngine() {
    Shutdown();
}

// ─── Initialize ──────────────────────────────────────────────────────────────

Result<void> EventEngine::Initialize(IUIAutomation* automation) {
    if (m_running.load()) {
        ACOS_LOG_WARNING(kComponent, "Initialize() called while already running");
        return Result<void>::Ok();
    }

    if (!automation) {
        return Result<void>::Fail(
            MakeError(ErrorCode::UIAutomationUnavailable),
            "Null IUIAutomation passed to EventEngine::Initialize");
    }

    m_automation = automation;
    ACOS_LOG_INFO(kComponent, "Initializing event engine");

    // Create the COM event sink.
    // UIAEventSink is COM ref-counted; we hold a raw pointer and call
    // Release() in Shutdown() to balance the initial ref of 1.
    m_sink = new UIAEventSink(m_automation, m_queue);

    // Register focus-changed event on the entire desktop tree.
    HRESULT hr = m_automation->AddFocusChangedEventHandler(
        nullptr,   // No cache request needed for snapshots
        m_sink);

    if (FAILED(hr)) {
        std::ostringstream oss;
        oss << "AddFocusChangedEventHandler failed: HRESULT=0x" << std::hex << hr;
        ACOS_LOG_ERROR(kComponent, oss.str());
        m_sink->Release();
        m_sink = nullptr;
        return Result<void>::Fail(
            MakeError(ErrorCode::UIAutomationUnavailable), oss.str());
    }

    // Register property-changed events for name and value on all elements.
    // We watch UIA_NamePropertyId and UIA_ValueValuePropertyId desktop-wide.
    PROPERTYID propertyIds[] = {
        UIA_NamePropertyId,
        UIA_ValueValuePropertyId,
        UIA_IsEnabledPropertyId,
    };
    hr = m_automation->AddPropertyChangedEventHandlerNativeArray(
        nullptr,   // Root element — desktop scope
        TreeScope_Subtree,
        nullptr,   // No cache request
        m_sink,
        propertyIds,
        ARRAYSIZE(propertyIds));

    if (FAILED(hr)) {
        // Property change events are best-effort — log but don't fail init.
        std::ostringstream oss;
        oss << "AddPropertyChangedEventHandler failed (non-fatal): HRESULT=0x"
            << std::hex << hr;
        ACOS_LOG_WARNING(kComponent, oss.str());
    }

    // Register UIA notification events (toasts, action center, app notifications).
    // UIA_NotificationEventId = 20035 — available on Windows 10 Creators Update+.
    // Best-effort: failure is non-fatal on older SDK / older Windows.
    {
        ComPtr<IUIAutomationElement> desktop;
        if (SUCCEEDED(m_automation->GetRootElement(&desktop)) && desktop) {
            hr = m_automation->AddAutomationEventHandler(
                UIA_NotificationEventId,
                desktop.Get(),
                TreeScope_Subtree,
                nullptr,   // no cache request
                m_sink);
            if (FAILED(hr)) {
                std::ostringstream oss;
                oss << "AddAutomationEventHandler(Notification) failed (non-fatal): "
                    << "HRESULT=0x" << std::hex << hr;
                ACOS_LOG_WARNING(kComponent, oss.str());
            }
        }
    }

    // Start the worker thread.
    m_running.store(true);
    m_worker = std::thread(&EventEngine::WorkerThread, this);

    ACOS_LOG_INFO(kComponent, "Event engine initialized and worker thread started");
    return Result<void>::Ok();
}

// ─── Shutdown ────────────────────────────────────────────────────────────────

void EventEngine::Shutdown() {
    if (!m_running.exchange(false)) return;

    ACOS_LOG_INFO(kComponent, "Shutting down event engine");

    // Unregister all UIA event handlers before stopping the queue.
    if (m_automation && m_sink) {
        m_automation->RemoveFocusChangedEventHandler(m_sink);
        m_automation->RemoveAllEventHandlers();
    }

    // Stop the queue — wakes the worker thread.
    m_queue->Stop();

    // Join the worker thread.
    if (m_worker.joinable()) {
        m_worker.join();
    }

    // Release the COM sink.
    if (m_sink) {
        m_sink->Release();
        m_sink = nullptr;
    }

    m_automation = nullptr;
    ACOS_LOG_INFO(kComponent, "Event engine shut down");
}

// ─── Listener management ─────────────────────────────────────────────────────

void EventEngine::AddListener(IEventListener* listener) {
    if (!listener) return;
    std::lock_guard<std::mutex> lock(m_listenerMutex);
    m_listeners.push_back(listener);
}

void EventEngine::RemoveListener(IEventListener* listener) {
    std::lock_guard<std::mutex> lock(m_listenerMutex);
    auto it = std::find(m_listeners.begin(), m_listeners.end(), listener);
    if (it != m_listeners.end()) {
        m_listeners.erase(it);
    }
}

size_t EventEngine::QueueDepth() const {
    return m_queue->Size();
}

// ─── Worker thread ───────────────────────────────────────────────────────────

void EventEngine::WorkerThread() {
    ACOS_LOG_INFO(kComponent, "Event worker thread started");

    while (!m_queue->IsStopped()) {
        auto evt = m_queue->Dequeue(std::chrono::milliseconds(100));
        if (!evt.has_value()) continue;
        DispatchEvent(evt.value());
    }

    // Drain any remaining events after stop.
    while (true) {
        auto evt = m_queue->Dequeue(std::chrono::milliseconds(0));
        if (!evt.has_value()) break;
        DispatchEvent(evt.value());
    }

    ACOS_LOG_INFO(kComponent, "Event worker thread exiting");
}

void EventEngine::DispatchEvent(const AccessEvent& event) {
    // Snapshot the listener list under the lock, then dispatch without holding it.
    // This avoids deadlocks if a listener calls AddListener/RemoveListener.
    std::vector<IEventListener*> listeners;
    {
        std::lock_guard<std::mutex> lock(m_listenerMutex);
        listeners = m_listeners;
    }

    for (IEventListener* listener : listeners) {
        listener->OnEvent(event);
    }
}

} // namespace AccessOS
