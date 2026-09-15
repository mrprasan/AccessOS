// AccessOS/src/Core/Events/UIAEventSink.cpp

#include "UIAEventSink.h"
#include "../Accessibility/UIAutomation/UIARoleMap.h"
#include "../Logging/Logger.h"

#include <sstream>
#include <chrono>

using Microsoft::WRL::ComPtr;

namespace AccessOS {

namespace {

static constexpr const char* kComponent = "UIAEventSink";

// Returns milliseconds since process start for event timestamps.
uint64_t NowMs() {
    using namespace std::chrono;
    static const auto kStart = steady_clock::now();
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(steady_clock::now() - kStart).count());
}

// Converts a BSTR to std::string (UTF-8). Returns empty on null.
std::string BstrToUtf8(BSTR bstr) {
    if (!bstr) return {};
    int len = ::WideCharToMultiByte(CP_UTF8, 0, bstr, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string s(static_cast<size_t>(len - 1), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, bstr, -1, s.data(), len, nullptr, nullptr);
    return s;
}

} // anonymous namespace

// ─── Constructor ─────────────────────────────────────────────────────────────

UIAEventSink::UIAEventSink(
    IUIAutomation* automation,
    std::shared_ptr<EventQueue> queue)
    : m_automation(automation)
    , m_queue(std::move(queue))
{
}

// ─── IUnknown ────────────────────────────────────────────────────────────────

HRESULT STDMETHODCALLTYPE UIAEventSink::QueryInterface(REFIID riid, void** ppv) {
    if (!ppv) return E_POINTER;

    if (riid == IID_IUnknown) {
        *ppv = static_cast<IUnknown*>(
            static_cast<IUIAutomationFocusChangedEventHandler*>(this));
    } else if (riid == IID_IUIAutomationFocusChangedEventHandler) {
        *ppv = static_cast<IUIAutomationFocusChangedEventHandler*>(this);
    } else if (riid == IID_IUIAutomationPropertyChangedEventHandler) {
        *ppv = static_cast<IUIAutomationPropertyChangedEventHandler*>(this);
    } else if (riid == IID_IUIAutomationStructureChangedEventHandler) {
        *ppv = static_cast<IUIAutomationStructureChangedEventHandler*>(this);
    } else if (riid == IID_IUIAutomationEventHandler) {
        *ppv = static_cast<IUIAutomationEventHandler*>(this);
    } else {
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE UIAEventSink::AddRef() {
    return ::InterlockedIncrement(&m_refCount);
}

ULONG STDMETHODCALLTYPE UIAEventSink::Release() {
    ULONG ref = ::InterlockedDecrement(&m_refCount);
    if (ref == 0) delete this;
    return ref;
}

// ─── Focus changed ───────────────────────────────────────────────────────────

HRESULT STDMETHODCALLTYPE UIAEventSink::HandleFocusChangedEvent(
    IUIAutomationElement* sender)
{
    // This is called on a UIA internal thread.
    // Snapshot the element immediately and post — do not do more work here.
    if (!sender || !m_queue) return S_OK;

    AccessEvent evt;
    evt.type        = AccessEventType::FocusChanged;
    evt.element     = SnapshotElement(sender);
    evt.timestampMs = NowMs();

    m_queue->Enqueue(std::move(evt));
    return S_OK;
}

// ─── Property changed ────────────────────────────────────────────────────────

HRESULT STDMETHODCALLTYPE UIAEventSink::HandlePropertyChangedEvent(
    IUIAutomationElement* sender,
    PROPERTYID            propertyId,
    VARIANT               /*newValue*/)
{
    if (!sender || !m_queue) return S_OK;

    AccessEventType evtType = PropertyIdToEventType(propertyId);
    if (evtType == AccessEventType::Unknown) return S_OK;

    AccessEvent evt;
    evt.type        = evtType;
    evt.element     = SnapshotElement(sender);
    evt.timestampMs = NowMs();

    m_queue->Enqueue(std::move(evt));
    return S_OK;
}

// ─── Structure changed ───────────────────────────────────────────────────────

HRESULT STDMETHODCALLTYPE UIAEventSink::HandleStructureChangedEvent(
    IUIAutomationElement* sender,
    StructureChangeType   /*changeType*/,
    SAFEARRAY*            /*runtimeId*/)
{
    if (!sender || !m_queue) return S_OK;

    AccessEvent evt;
    evt.type        = AccessEventType::StructureChanged;
    evt.element     = SnapshotElement(sender);
    evt.timestampMs = NowMs();

    m_queue->Enqueue(std::move(evt));
    return S_OK;
}

// ─── Private helpers ─────────────────────────────────────────────────────────

AccessNode UIAEventSink::SnapshotElement(IUIAutomationElement* element) {
    AccessNode node;
    if (!element) return node;

    node.provider = AccessProvider::UIAutomation;
    node.isValid  = true;

    // Role
    CONTROLTYPEID ctid = 0;
    if (SUCCEEDED(element->get_CurrentControlType(&ctid))) {
        node.role = UIARoleMap::FromControlType(ctid);
    }

    // Name
    {
        BSTR bstr = nullptr;
        if (SUCCEEDED(element->get_CurrentName(&bstr)) && bstr) {
            node.name = BstrToUtf8(bstr);
            ::SysFreeString(bstr);
        }
    }

    // Focus state
    BOOL hasFocus = FALSE;
    if (SUCCEEDED(element->get_CurrentHasKeyboardFocus(&hasFocus)) && hasFocus) {
        node.state = node.state | AccessState::Focused;
    }

    BOOL canFocus = FALSE;
    if (SUCCEEDED(element->get_CurrentIsKeyboardFocusable(&canFocus)) && canFocus) {
        node.state = node.state | AccessState::Focusable;
    }

    // Password — mark Protected so logging layer never logs content
    BOOL isPassword = FALSE;
    if (SUCCEEDED(element->get_CurrentIsPassword(&isPassword)) && isPassword) {
        node.state = node.state | AccessState::Protected;
    }

    // Process ID
    int pid = 0;
    if (SUCCEEDED(element->get_CurrentProcessId(&pid))) {
        node.processId = static_cast<uint32_t>(pid);
    }

    // Bounds
    RECT rect{};
    if (SUCCEEDED(element->get_CurrentBoundingRectangle(&rect))) {
        node.bounds = { rect.left, rect.top,
                        rect.right - rect.left, rect.bottom - rect.top };
    }

    // Element ID via RuntimeId hash (FNV-1a)
    SAFEARRAY* runtimeId = nullptr;
    if (SUCCEEDED(element->GetRuntimeId(&runtimeId)) && runtimeId) {
        uint64_t hash = 14695981039346656037ULL;
        constexpr uint64_t prime = 1099511628211ULL;
        LONG lb = 0, ub = 0;
        ::SafeArrayGetLBound(runtimeId, 1, &lb);
        ::SafeArrayGetUBound(runtimeId, 1, &ub);
        for (LONG i = lb; i <= ub; ++i) {
            int val = 0;
            ::SafeArrayGetElement(runtimeId, &i, &val);
            const auto* b = reinterpret_cast<const uint8_t*>(&val);
            for (int j = 0; j < 4; ++j) { hash ^= b[j]; hash *= prime; }
        }
        ::SafeArrayDestroy(runtimeId);
        node.id = hash;
    }

    return node;
}

AccessEventType UIAEventSink::PropertyIdToEventType(PROPERTYID id) noexcept {
    switch (id) {
    case UIA_NamePropertyId:             return AccessEventType::NameChanged;
    case UIA_ValueValuePropertyId:       return AccessEventType::ValueChanged;
    case UIA_IsEnabledPropertyId:        return AccessEventType::StateChanged;
    case UIA_ToggleToggleStatePropertyId:return AccessEventType::StateChanged;
    case UIA_HelpTextPropertyId:         return AccessEventType::DescriptionChanged;
    default:                             return AccessEventType::Unknown;
    }
}

} // namespace AccessOS

// ─── IUIAutomationEventHandler (generic events — ACCESSOS-035) ───────────────

HRESULT STDMETHODCALLTYPE AccessOS::UIAEventSink::HandleAutomationEvent(
    IUIAutomationElement* sender,
    EVENTID               eventId)
{
    if (!sender) return S_OK;

    AccessEvent evt;
    evt.element     = SnapshotElement(sender);
    evt.timestampMs = NowMs();

    switch (eventId) {
    case UIA_NotificationEventId:
        evt.type = AccessEventType::NotificationRaised;
        break;
    case UIA_MenuOpenedEventId:
        evt.type = AccessEventType::MenuOpened;
        break;
    case UIA_MenuClosedEventId:
        evt.type = AccessEventType::MenuClosed;
        break;
    case UIA_Window_WindowOpenedEventId:
        evt.type = AccessEventType::WindowOpened;
        break;
    case UIA_Window_WindowClosedEventId:
        evt.type = AccessEventType::WindowClosed;
        break;
    default:
        return S_OK; // ignore unrecognised events
    }

    if (m_queue) m_queue->Enqueue(evt);
    return S_OK;
}
