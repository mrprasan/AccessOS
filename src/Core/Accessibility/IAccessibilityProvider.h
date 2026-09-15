// AccessOS/src/Core/Accessibility/IAccessibilityProvider.h
//
// Provider abstraction — hides UIA, MSAA, IA2, Browser, and OCR details
// from the semantic engine.
//
// Why: The architecture requires that raw provider objects never flow beyond
//      their acquisition layer. All consumers work against this interface.
//
// Contract:
//   - Implementations must be constructed on the thread that owns the
//     provider (typically the accessibility event thread).
//   - GetFocusedElement() may be called from any thread but must
//     internally marshal to the correct thread.
//   - Returned AccessNode objects are snapshots — they do not hold live
//     COM pointers or other provider handles.
//
// Threading: See individual method documentation.
//
// Future extension: Add BrowserProvider, OCRProvider as new implementations.

#pragma once

#include "../Semantic/AccessNode.h"
#include "../Error/AccessError.h"

#include <functional>
#include <memory>

namespace AccessOS {

// Callback invoked when the focused element changes.
// Called on the provider's internal thread — do not perform expensive
// work inside this callback. Post to a queue instead.
using FocusChangedCallback = std::function<void(AccessNode)>;

// Callback invoked on structural or property changes.
using ElementChangedCallback = std::function<void(AccessNode)>;

// ─── IAccessibilityProvider ───────────────────────────────────────────────────
class IAccessibilityProvider {
public:
    virtual ~IAccessibilityProvider() = default;

    // Initialize the provider. Must be called before any other method.
    // Returns an error if the provider cannot be initialized.
    virtual Result<void> Initialize() = 0;

    // Release provider resources. Safe to call multiple times.
    virtual void Shutdown() = 0;

    // Returns the provider type for diagnostic purposes.
    virtual AccessProvider ProviderType() const noexcept = 0;

    // Returns true if the provider is ready to serve requests.
    virtual bool IsAvailable() const noexcept = 0;

    // Returns the currently focused accessible element.
    // Returns an error if focus cannot be determined.
    // Threading: May block briefly while querying the provider.
    virtual Result<AccessNode> GetFocusedElement() = 0;

    // Returns the root desktop element.
    virtual Result<AccessNode> GetRootElement() = 0;

    // Returns the element at the given screen coordinates.
    virtual Result<AccessNode> GetElementAtPoint(int x, int y) = 0;

    // Returns the parent of the given element.
    virtual Result<AccessNode> GetParent(uint64_t elementId) = 0;

    // Returns the children of the given element.
    virtual Result<std::vector<AccessNode>> GetChildren(uint64_t elementId) = 0;

    // Register a callback for focus-change events.
    // Only one callback is supported per provider instance.
    virtual void SetFocusChangedCallback(FocusChangedCallback callback) = 0;

    // Register a callback for element property/structure changes.
    virtual void SetElementChangedCallback(ElementChangedCallback callback) = 0;
};

} // namespace AccessOS
