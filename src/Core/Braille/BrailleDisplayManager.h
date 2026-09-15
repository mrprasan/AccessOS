#pragma once
// BrailleDisplayManager.h — manages one active Braille display (ACCESSOS-021)
//
// Responsibilities:
//   - Holds a reference to the active IBrailleDisplay
//   - Translates text → cells via BrailleTranslator
//   - Manages a viewport: pan offset into a longer cell buffer
//   - Writes the current viewport window to the display

#pragma once
#include "IBrailleDisplay.h"
#include "BrailleTranslator.h"
#include <memory>
#include <mutex>

namespace AccessOS {
namespace Braille {

class BrailleDisplayManager {
public:
    BrailleDisplayManager();
    explicit BrailleDisplayManager(std::shared_ptr<IBrailleDisplay> display);

    // Attach / detach display
    void SetDisplay(std::shared_ptr<IBrailleDisplay> display);
    std::shared_ptr<IBrailleDisplay> GetDisplay() const;
    bool HasDisplay() const;

    // Write text to the display (translated to Braille, viewport at offset 0)
    // Returns false if no display connected or translation is empty.
    bool WriteText(const std::string& text);

    // Write pre-translated cells directly (viewport at offset 0)
    bool WriteCells(const std::vector<BrailleCell>& cells);

    // Pan viewport one display-width left / right.
    // Returns false if at boundary or no display.
    bool PanLeft();
    bool PanRight();

    // Current pan offset (index into m_buffer)
    uint32_t GetPanOffset() const;

    // Current cell buffer (full translated content, not just the viewport)
    const std::vector<BrailleCell>& GetBuffer() const;

    // Move cursor to cell index within the buffer (may pan to make it visible)
    bool SetCursorCell(uint32_t cellIndex);

    // Flush current viewport window to the display
    bool Flush();

private:
    std::shared_ptr<IBrailleDisplay> m_display;
    BrailleTranslator                m_translator;
    std::vector<BrailleCell>         m_buffer;     // full translated cell buffer
    uint32_t                         m_panOffset;  // current viewport start index
    mutable std::mutex               m_mutex;

    // Write m_buffer[m_panOffset .. m_panOffset+cellCount) to display
    bool FlushLocked();
    uint32_t CellCount() const; // display cell count, or 0
};

} // namespace Braille
} // namespace AccessOS
