// AccessOS/src/Core/Events/UIAEventSink.h
//
// COM event sink — implements IUIAutomationFocusChangedEventHandler
// and posts normalized events to the EventQueue.
//
// Why: UIA requires a COM object to receive events. This sink is the
//      only component that holds live UIA pointers in callbacks.
//      It converts them to AccessNode snapshots and immediately posts
//      to the queue, keeping the callback as short as possible.
//
// Threading: HandleFocusChangedEvent() is called on a UIA internal thread.
//            It must return quickly — all work beyond snapshotting is
//            done on the EventQueue worker thread.
//
// Lifetime: UIAEventSink is reference-counted via COM (IUnknown).
//           The EventEngine holds a ComPtr<UIAEventSink>.

#pragma once

#include "EventQueue.h"
#include "../Accessibility/UIAutomation/UIAIncludes.h"

#include <memory>

namespace AccessOS {

class UIAEventSink final
    : public IUIAutomationFocusChangedEventHandler
    , public IUIAutomationPropertyChangedEventHandler
    , public IUIAutomationStructureChangedEventHandler
    , public IUIAutomationEventHandler
{
public:
    explicit UIAEventSink(
        IUIAutomation* automation,
        std::shared_ptr<EventQueue> queue);

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
    ULONG   STDMETHODCALLTYPE AddRef()  override;
    ULONG   STDMETHODCALLTYPE Release() override;

    // IUIAutomationFocusChangedEventHandler
    HRESULT STDMETHODCALLTYPE HandleFocusChangedEvent(
        IUIAutomationElement* sender) override;

    // IUIAutomationPropertyChangedEventHandler
    HRESULT STDMETHODCALLTYPE HandlePropertyChangedEvent(
        IUIAutomationElement* sender,
        PROPERTYID            propertyId,
        VARIANT               newValue) override;

    // IUIAutomationStructureChangedEventHandler
    HRESULT STDMETHODCALLTYPE HandleStructureChangedEvent(
        IUIAutomationElement*   sender,
        StructureChangeType     changeType,
        SAFEARRAY*              runtimeId) override;

    // IUIAutomationEventHandler (generic events — notification, menu, window)
    HRESULT STDMETHODCALLTYPE HandleAutomationEvent(
        IUIAutomationElement* sender,
        EVENTID               eventId) override;

private:
    // Converts a live UIA element to an AccessNode snapshot.
    // Called on the UIA callback thread — must be fast.
    AccessNode SnapshotElement(IUIAutomationElement* element);

    // Converts UIA PropertyId to AccessEventType.
    static AccessEventType PropertyIdToEventType(PROPERTYID id) noexcept;

    IUIAutomation*                  m_automation;  // Non-owning — owned by UIAProvider
    std::shared_ptr<EventQueue>     m_queue;
    LONG                            m_refCount{ 1 };
};

} // namespace AccessOS
