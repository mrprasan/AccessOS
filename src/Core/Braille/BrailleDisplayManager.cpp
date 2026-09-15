// BrailleDisplayManager.cpp — manages one active Braille display (ACCESSOS-021)
#include "BrailleDisplayManager.h"
#include <algorithm>
#include <climits>

namespace AccessOS {
namespace Braille {

BrailleDisplayManager::BrailleDisplayManager()
    : m_panOffset(0) {}

BrailleDisplayManager::BrailleDisplayManager(std::shared_ptr<IBrailleDisplay> display)
    : m_display(std::move(display)), m_panOffset(0) {}

void BrailleDisplayManager::SetDisplay(std::shared_ptr<IBrailleDisplay> display) {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_display = std::move(display);
    m_panOffset = 0;
}

std::shared_ptr<IBrailleDisplay> BrailleDisplayManager::GetDisplay() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_display;
}

bool BrailleDisplayManager::HasDisplay() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_display && m_display->IsConnected();
}

bool BrailleDisplayManager::WriteText(const std::string& text) {
    auto cells = m_translator.Translate(text);
    if (cells.empty()) return false;
    std::lock_guard<std::mutex> lk(m_mutex);
    if (!m_display || !m_display->IsConnected()) return false;
    m_buffer    = std::move(cells);
    m_panOffset = 0;
    return FlushLocked();
}

bool BrailleDisplayManager::WriteCells(const std::vector<BrailleCell>& cells) {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (!m_display || !m_display->IsConnected()) return false;
    m_buffer    = cells;
    m_panOffset = 0;
    return FlushLocked();
}

bool BrailleDisplayManager::PanLeft() {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (!m_display || !m_display->IsConnected()) return false;
    uint32_t step = CellCount();
    if (step == 0 || m_panOffset == 0) return false;
    m_panOffset = (m_panOffset >= step) ? (m_panOffset - step) : 0;
    return FlushLocked();
}

bool BrailleDisplayManager::PanRight() {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (!m_display || !m_display->IsConnected()) return false;
    uint32_t step = CellCount();
    if (step == 0) return false;
    uint32_t bufSize = static_cast<uint32_t>(m_buffer.size());
    if (m_panOffset + step >= bufSize) return false; // already at or past end
    m_panOffset += step;
    return FlushLocked();
}

uint32_t BrailleDisplayManager::GetPanOffset() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_panOffset;
}

const std::vector<BrailleCell>& BrailleDisplayManager::GetBuffer() const {
    // Caller must not hold mutex; safe for single-threaded test use
    return m_buffer;
}

bool BrailleDisplayManager::SetCursorCell(uint32_t cellIndex) {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (!m_display || !m_display->IsConnected()) return false;
    uint32_t step = CellCount();
    if (step > 0) {
        // Pan so that cellIndex is visible
        m_panOffset = (cellIndex / step) * step;
    }
    return m_display->SetCursorCell(cellIndex) && FlushLocked();
}

bool BrailleDisplayManager::Flush() {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (!m_display || !m_display->IsConnected()) return false;
    return FlushLocked();
}

// ── private ───────────────────────────────────────────────────────────────────

bool BrailleDisplayManager::FlushLocked() {
    uint32_t count = CellCount();
    if (count == 0) return false;
    uint32_t bufSize = static_cast<uint32_t>(m_buffer.size());
    // Build the viewport window
    std::vector<BrailleCell> window(count, 0x00);
    for (uint32_t j = 0; j < count; ++j) {
        uint32_t src = m_panOffset + j;
        if (src < bufSize) window[j] = m_buffer[src];
    }
    return m_display->Write(window);
}

uint32_t BrailleDisplayManager::CellCount() const {
    if (!m_display) return 0;
    return m_display->GetCapabilities().cellCount;
}

} // namespace Braille
} // namespace AccessOS
