#pragma once
// IBrailleDisplay.h — Abstract Braille display interface (ACCESSOS-021)
//
// Models an external refreshable Braille display device.
// Concrete implementations: StubBrailleDisplay (tests), HidBrailleDisplay (hardware).

#pragma once
#include "BrailleCells.h"
#include <string>
#include <vector>
#include <cstdint>

namespace AccessOS {
namespace Braille {

// Display capabilities descriptor returned by GetCapabilities()
struct BrailleDisplayCaps {
    uint32_t cellCount     = 0;   // number of refreshable cells (e.g. 40, 80)
    bool     hasCursor     = false;
    bool     hasRoutingKeys= false;
    bool     hasPanKeys    = false;
    std::string deviceName;       // human-readable device identifier
};

// Cursor routing event: physical key index (0-based) pressed above a cell
struct RoutingKeyEvent {
    uint32_t cellIndex = 0;
};

class IBrailleDisplay {
public:
    virtual ~IBrailleDisplay() = default;

    // Connect / disconnect lifecycle
    virtual bool Connect()    = 0;
    virtual void Disconnect() = 0;
    virtual bool IsConnected() const = 0;

    // Device info
    virtual BrailleDisplayCaps GetCapabilities() const = 0;

    // Write cells to the display starting at offset 0.
    // Cells beyond cellCount are silently ignored.
    // Returns false if not connected or write fails.
    virtual bool Write(const std::vector<BrailleCell>& cells) = 0;

    // Pan the viewport left/right by one display width.
    // Returns false if panning is not supported or at the boundary.
    virtual bool PanLeft()  = 0;
    virtual bool PanRight() = 0;

    // Move cursor cell to the given 0-based index (cursor routing simulation).
    virtual bool SetCursorCell(uint32_t cellIndex) = 0;

    // Current cursor position (0-based), or UINT32_MAX if not set
    virtual uint32_t GetCursorCell() const = 0;
};

} // namespace Braille
} // namespace AccessOS
