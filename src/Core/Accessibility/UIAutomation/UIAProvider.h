// AccessOS/src/Core/Accessibility/UIAutomation/UIAProvider.h
//
// UI Automation provider — primary Windows accessibility source.
//
// Why: UIA is the preferred accessibility API on modern Windows.
//      It provides structured, event-driven access to application UI
//      without requiring application-specific code.
//
// Responsibilities:
//   - Initialize and own the IUIAutomation COM interface
//   - Discover the focused element
//   - Read element properties into AccessNode
//   - Register and dispatch UIA event callbacks
//
// Must NOT:
//   - Perform speech, navigation, or presentation logic
//   - Expose raw IUIAutomationElement pointers outside this class
//   - Block the accessibility event thread with expensive work
//
// Threading:
//   - Initialize() and Shutdown() must be called from the same thread
//     that owns the COM apartment (MTA recommended for UIA).
//   - GetFocusedElement() is safe to call from any thread — it
//     marshals internally via the COM proxy.
//   - Event callbacks arrive on a UIA internal thread. The callback
//     implementation posts to a queue and returns immediately.
//
// COM apartment:
//   UIA works best in a Multithreaded Apartment (MTA).
//   The caller must CoInitializeEx(nullptr, COINIT_MULTITHREADED)
//   before calling Initialize().

#pragma once

#include "../IAccessibilityProvider.h"
#include "UIAIncludes.h"

#include <atomic>
#include <mutex>

namespace AccessOS {

class UIAProvider final : public IAccessibilityProvider {
public:
    UIAProvider();
    ~UIAProvider() override;

    // IAccessibilityProvider
    Result<void>                    Initialize()                          override;
    void                            Shutdown()                            override;
    AccessProvider                  ProviderType()      const noexcept   override;
    bool                            IsAvailable()       const noexcept   override;
    Result<AccessNode>              GetFocusedElement()                   override;
    Result<AccessNode>              GetRootElement()                      override;
    Result<AccessNode>              GetElementAtPoint(int x, int y)       override;
    Result<AccessNode>              GetParent(uint64_t elementId)         override;
    Result<std::vector<AccessNode>> GetChildren(uint64_t elementId)       override;
    void SetFocusChangedCallback(FocusChangedCallback callback)           override;
    void SetElementChangedCallback(ElementChangedCallback callback)       override;

private:
    // Converts a live IUIAutomationElement into a provider-independent
    // AccessNode snapshot. Returns an error if the element is stale.
    Result<AccessNode> ElementToNode(IUIAutomationElement* element);

    // Maps a UIA ControlType identifier to the AccessOS AccessRole enum.
    // Defined separately to keep the mapping table readable.
    static AccessRole ControlTypeToRole(CONTROLTYPEID controlType) noexcept;

    // Reads the accessible name from the element.
    // Tries Name property first; falls back to LegacyIAccessible.
    static std::string ReadName(IUIAutomationElement* element);

    // Reads element state flags into AccessState.
    static AccessState ReadState(IUIAutomationElement* element);

    // Generates a stable-enough ID for an element within a session.
    // Uses the UIA RuntimeId array hashed to a uint64_t.
    static uint64_t ComputeElementId(IUIAutomationElement* element);

    Microsoft::WRL::ComPtr<IUIAutomation>  m_automation;
    std::atomic<bool>                      m_initialized{ false };

    FocusChangedCallback                   m_focusCallback;
    ElementChangedCallback                 m_elementCallback;
    mutable std::mutex                     m_callbackMutex;
};

} // namespace AccessOS
