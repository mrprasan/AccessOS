#pragma once
// IAccessAdapter.h — abstract adapter interface (ACCESSOS-023)
//
// An adapter connects one external accessibility source to the AccessOS pipeline.
// Concrete adapters:
//   UiaAdapter     — Windows UI Automation event stream
//   BrowserAdapter — Native Messaging Host (browser extension focus events)
//
// Lifecycle:
//   Attach()   — start receiving events from the source
//   Detach()   — stop receiving events, release resources
//   IsAttached — query current state
//
// Events are delivered via AdapterEventCallback (set once before Attach).

#include <string>
#include <functional>
#include <cstdint>

namespace AccessOS {
namespace Adapters {

// Adapter event kinds delivered to the pipeline
enum class AdapterEventKind : uint32_t {
    FocusChanged   = 0,
    PropertyChange = 1,
    StructureChange= 2,
    BrowserFocus   = 3,
    BrowserPageLoad= 4,
    Alert          = 5,
    Custom         = 99,
};

// Lightweight event carried from adapter → pipeline
struct AdapterEvent {
    AdapterEventKind kind         = AdapterEventKind::FocusChanged;
    std::string      sourceName;   // adapter name that produced the event
    std::string      elementName;  // accessible name of the element (if known)
    std::string      elementRole;  // role string (if known)
    std::string      extraData;    // JSON or plain text payload (optional)
    uint64_t         timestampMs  = 0;
};

using AdapterEventCallback = std::function<void(const AdapterEvent&)>;

class IAccessAdapter {
public:
    virtual ~IAccessAdapter() = default;

    // Human-readable adapter identifier (e.g. "UIA", "Browser")
    virtual std::string GetName() const = 0;

    // Set the callback that receives events (must be called before Attach)
    virtual void SetEventCallback(AdapterEventCallback cb) = 0;

    // Start the adapter (subscribe to the source)
    // Returns true if successfully attached.
    virtual bool Attach() = 0;

    // Stop the adapter (unsubscribe, release handles)
    virtual void Detach() = 0;

    // Query whether currently attached
    virtual bool IsAttached() const = 0;
};

} // namespace Adapters
} // namespace AccessOS
