// AccessOS/src/Core/Events/IEventListener.h
//
// Event listener interface — consumers implement this to receive
// normalized AccessOS events from the event engine.
//
// Why: The event engine must not know about speech, navigation, or UI.
//      Listeners decouple producers from consumers. Each subsystem
//      (FocusManager, ContextEngine, etc.) registers as a listener.
//
// Threading: OnEvent() is called on the event engine's worker thread.
//            Implementations must not block. Post to their own queue if needed.

#pragma once

#include "../Semantic/AccessNode.h"
#include <cstdint>
#include <string>

namespace AccessOS {

// Normalized event types — independent of UIA event IDs.
enum class AccessEventType {
    Unknown = 0,

    // Focus
    FocusChanged,           // A different element received focus

    // Structure
    StructureChanged,       // Child elements added or removed

    // Property
    NameChanged,            // Accessible name changed
    ValueChanged,           // Element value changed
    StateChanged,           // Element state changed (enabled, checked, etc.)
    DescriptionChanged,     // Accessible description changed

    // Selection
    SelectionChanged,       // Selection within a container changed
    SelectionItemAdded,
    SelectionItemRemoved,

    // Content
    LiveRegionChanged,      // ARIA live region or UIA notification
    TextChanged,            // Text content changed in an edit control
    NotificationRaised,     // App-level notification (UIA_NotificationEventId)

    // Window
    WindowOpened,
    WindowClosed,
    WindowActivated,

    // Menu
    MenuOpened,
    MenuClosed,
    MenuItemInvoked,
};

// A normalized accessibility event — provider-independent.
struct AccessEvent {
    AccessEventType type    = AccessEventType::Unknown;
    AccessNode      element;        // Snapshot of the affected element
    std::string     extraInfo;      // Optional supplementary data (e.g. notification text)
    uint64_t        timestampMs = 0; // Milliseconds since process start
};

// Listener interface — implement to receive events from the event engine.
class IEventListener {
public:
    virtual ~IEventListener() = default;

    // Called on the event engine worker thread when a normalized event arrives.
    // Must return quickly — do not perform speech, UIA queries, or UI updates here.
    virtual void OnEvent(const AccessEvent& event) = 0;
};

} // namespace AccessOS
